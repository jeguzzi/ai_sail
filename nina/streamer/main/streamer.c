/*
 * Copyright (C) 2019 GreenWaves Technologies
 * Copyright (C) 2020 Bitcraze AB
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Authors:  Esteban Gougeon, GreenWaves Technologies (esteban.gougeon@greenwaves-technologies.com)
 *           Germain Haugou, GreenWaves Technologies (germain.haugou@greenwaves-technologies.com)
 */

/*
 * Nina firmware for the AI-deck streaming JPEG demo. This demo takes
 * JPEG data sent from the GAP8 and forwards it to a TCP socket. The data
 * sent on the socket is a continous stream of JPEG images, where the JPEG
 * start-of-frame (0xFF 0xD8) and end-of-frame (0xFF 0xD9) is used to
 * pick out images from the stream.
 *
 * The Frame Streamer on the GAP8 will start sending data once it's booted.
 * This firmware only uses the JPEG data sent by the Frame Streamer and
 * ignores the rest.
 *
 * The GAP8 communication sequence is described below:
 *
 * GAP8 sends 4 32-bit unsigned integers (nina_req_t) where:
 *  * type - Describes the type of package (0x81 is JPEG data)
 *  * size - The size of data that should be requested
 *  * info - Used for signaling end-of-frame of the JPEG
 *
 * When the frame streamer starts it will send the following packages of type 0x81:
 * 1) The JPEG header (306 bytes hardcoded in the GAP8 SDK) (info == 1)
 * 2) The JPEG footer (2 bytes hardcoded in the GAP8 SDK) (info == 1)
 * 3) Continous JPEG data (excluding header and footer) where info == 0 for everything
 *    except the package with the last data of a frame
 *
 * Note that if you reflash/restart the Nina you will have to restart the GAP8, since it
 * only sends the JPEG header/footer on startup!
 *
 * This firmware will save the JPEG header and footer and send this on the socket
 * bofore and after each frame that is received from the GAP8.
 *
 * Tested with GAP8 SEK version 3.4.

 ****************************************************************************************
 * This is a modification of the above code, which is provided as an example by Bitcraze:
  - added support for RAW streaming
  - added support for deep sleeping until waken up by GAP through a GPIO HIGH
  - changed the LED pattern

  Author: Jérôme Guzzi, jerome@idsia.ch
 ****************************************************************************************
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp32/ulp.h"
#include "driver/touch_pad.h"
#include "driver/adc.h"
#include "driver/rtc_io.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "soc/rtc.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "wifi.h"
#include "spi.h"

#define RTC_REGISTER_ADDRESS_BANK_1 ((int *) 0x3FF4804C)
#define RTC_REGISTER_ADDRESS_BANK_2 ((int *) 0x3FF480B0)
#define SLEEP_TIME_USECONDS (1 * 1000000)

#define SLEEP_AFTER_BOOT
// #define SLEEP_IF_INACTIVE
// #define SLEEP_IF_GPIO_LOW
/* The GAP is connected also on a GPIO  (there is another one too) */
#define GAP_GPIO_IN 2

/* Log tag for printouts */
static const char *TAG = "streamer";

// If we are of no use, better to sleep and spare energy
void go_to_sleep()
{
  if(!rtc_gpio_is_valid_gpio(GAP_GPIO_IN))
  {
    ESP_LOGE(TAG, "GPIO %d cannot be used to wake up NINA.", GAP_GPIO_IN);
    return;
  }
  esp_sleep_enable_ext0_wakeup(GAP_GPIO_IN, 1);
  esp_wifi_stop();
  ESP_LOGI(TAG, "Going to deep sleep ... will wait for reboot or high on pin %d", GAP_GPIO_IN);
  esp_deep_sleep_start();
}

/* GAP8 streamer packet type for JPEG related data */
#define NINA_W10_CMD_SEND_PACKET  0x81

// Force AP despite of config ... does not work!!!
// #ifndef CONFIG_USE_AS_AP
// #define CONFIG_USE_AS_AP
// #endif

/* WiFi SSID/password and AP/station is set from menuconfig */
#ifdef CONFIG_USE_AS_AP
#define WIFI_MODE AIDECK_WIFI_MODE_SOFTAP
#define CONFIG_EXAMPLE_SSID NULL
#define CONFIG_EXAMPLE_PASSWORD NULL
#else
#define WIFI_MODE AIDECK_WIFI_MODE_STATION
#endif

/* The LED is connected on a GPIO */
#define BLINK_GPIO 4


/* The periods of the LED blinking */
static volatile uint32_t blink_period_ms_1 = 500;
static volatile uint32_t blink_period_ms_2 = 500;

static volatile int connected = 0;
static volatile int source_alive = 0;
static volatile int got_msg = 0;

static void update_led_pattern()
{
  if(source_alive)
  {
    if(connected)
    {
      blink_period_ms_1 = blink_period_ms_2 = 100;
    }
    else{
      blink_period_ms_1 = blink_period_ms_2 = 500;
    }
  }
  else{
    if(connected)
    {
      blink_period_ms_1 = 200;
      blink_period_ms_2 = 1000;
    }
    else{
      blink_period_ms_1 = blink_period_ms_2 = 2000;
    }
  }
}

/* GAP8 communication structs */

typedef struct
{
  uint32_t type; /* Is 0x81 for JPEG related data */
  uint32_t size; /* Size of data to request */
} __attribute__((packed)) nina_req_t;

typedef struct
{
  uint32_t channel;
  uint32_t size;
  uint32_t info;
} __attribute__((packed)) header_t;

/* Pointer to data transferred via SPI */
static uint8_t *spi_buffer;

/* Storage for JPEG header (hardcoded to 306 bytes like in GAP8) */
static uint8_t jpeg_header[306];
/* Storage for JPEG footer (hardcoded to 2 bytes like in GAP8) */
static uint8_t jpeg_footer[2];

/* JPEG communication state (see above) */
typedef enum
{
  HEADER, /* Next packet is header */
  FOOTER, /* Next packet is footer */
  DATA /* Next packet is data */
} ImageState_t;

/* JPEG communication state */
static ImageState_t state = HEADER;


// Raw protocol

#define RAW_HEADER "?IMG"
uint16_t raw_header[5];
const char raw_footer[] = "!IMG";
static bool new_frame = 0;

/* Do we know which image format to receive and forward? */

typedef enum
{
  RAW,
  JPEG,
  UNKNOWN
} image_format_t;

static image_format_t format = UNKNOWN;
static uint32_t number_of_frames = 0;

static void handle_gap8_package(uint8_t *buffer, int32_t length) {
  got_msg = 1;
  nina_req_t *req = (nina_req_t *)buffer;
  uint32_t *hs = (uint32_t *)buffer;
  // if(format == UNKNOWN)
  // {
  //   printf("handle_gap8_package (%d): %d %d %d ...\n", length, hs[0], hs[1], hs[2]);
  // }
  int size;
  int bs = 0;
  switch (req->type)
  {
    case 1:
      if(length > 95)
      {
        format = (image_format_t) hs[3];
        state = HEADER;
        switch (format) {
          case RAW:
            ESP_LOGI(TAG, "Got streaming request for RAW images");
            break;
          case JPEG:
            ESP_LOGI(TAG, "Got streaming request for RAW images");
            break;
          default:
            ESP_LOGI(TAG, "Got streaming request for unknown format %d", format);
            format = UNKNOWN;
        }
      }
      break;
    case NINA_W10_CMD_SEND_PACKET:
      if(req->size == 12)
      {
        length = spi_read_data(&buffer, req->size * 8);
        header_t *header = (header_t *) buffer;
        if(header->info <= 1 && format == JPEG)
        {
          new_frame = (header->info == 1);
          if(new_frame) number_of_frames++;
        }
        else if (header->info > 1 && format == RAW){
          // RAW
          raw_header[2] = (int16_t) (0xFFFF & header->info);
          raw_header[3] = (int16_t) (header->size / raw_header[2]);
          raw_header[4] = (int16_t) (header->info >> 16);
          if(number_of_frames == 0)
          {
            ESP_LOGI(TAG, "First header for RAW images: shape (%d, %d), color %d\n",
                     raw_header[2], raw_header[3], raw_header[4]);
          }
          number_of_frames++;
          if(wifi_is_socket_connected())
            wifi_send_packet((char *)raw_header, 10);
        }
        else{
          ESP_LOGE(TAG, "Unexpected header [%d, %d, %d]", header->channel, header->size, header->info);
        }
      }
      else{
        // Read image of type `req->type` and size `req->size`  bytes
        // size to read in bits
        size = req->size * 8;
        if(format == RAW)
        {
          while(size > 0)
          {
            bs = 8192;
            if(size < bs) bs = size;
            length = spi_read_data(&buffer, bs);
            // There are still `size - length` bits to read
            size -= length;
            if(wifi_is_socket_connected())
              wifi_send_packet((const char*) buffer, length / 8);
          }
          if(wifi_is_socket_connected())
            wifi_send_packet((const char*) raw_footer, 4);
        }
        else if(format == JPEG)
        {
          length = spi_read_data(&buffer, size);
          switch (state) {
            case HEADER:
              ESP_LOGI(TAG, "Setting JPEG header of %i bytes", length / 8);
              memcpy(&jpeg_header, (uint8_t*) buffer, sizeof(jpeg_header));
              if(wifi_is_socket_connected())
                wifi_send_packet( (const char*) &jpeg_header, sizeof(jpeg_header));
              state = FOOTER;
              break;
            case FOOTER:
              ESP_LOGI(TAG, "Setting JPEG footer of %i bytes", length / 8);
              memcpy(&jpeg_footer, (uint8_t*) buffer, sizeof(jpeg_footer));
              state = DATA;
              break;
            case DATA:
              if(wifi_is_socket_connected())
              {
                wifi_send_packet( (const char*) buffer, length / 8);
                if (new_frame) {
                  wifi_send_packet( (const char*) &jpeg_footer, sizeof(jpeg_footer) );
                  wifi_send_packet( (const char*) &jpeg_header, sizeof(jpeg_header) );
                }
              }
              break;
            }
          }
        }
      break;
  }
}

/* Task for receiving image data from GAP8 and sending to client via WiFi */
static void img_sending_task(void *pvParameters) {
  spi_init();
  memcpy(raw_header, RAW_HEADER, 4);
  while (1)
  {
    int32_t datalength = spi_read_data(&spi_buffer, CMD_PACKET_SIZE);
    if (datalength > 0) {
      handle_gap8_package(spi_buffer, datalength);
    }
  }
}

/* Task for handling WiFi state */
static void wifi_task(void *pvParameters) {
  wifi_bind_socket();
  while (1) {
    wifi_wait_for_socket_connected();
    connected = 1;
    update_led_pattern();
    wifi_wait_for_disconnect();
    connected = 0;
    update_led_pattern();
    ESP_LOGI(TAG, "Client disconnected");
  }
}

/* Task tp blick the LED */
static void led_task(void *pvParameters) {
  while(1)
  {
    gpio_set_level(BLINK_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(blink_period_ms_1));
    gpio_set_level(BLINK_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(blink_period_ms_2));
  }
}

void app_main()
{
  int wakeup_cause = esp_sleep_get_wakeup_cause();
  nvs_flash_init();
  ESP_LOGI(TAG, "Wake up cause: %d\n", wakeup_cause);

#ifdef SLEEP_AFTER_BOOT
  if(wakeup_cause != 2)
  {
    go_to_sleep();
  }
#endif

  xTaskCreate(img_sending_task, "img_sending_task", 4096, NULL, 5, NULL);

#ifdef SLEEP_IF_INACTIVE
  ESP_LOGI(TAG, "Wait 2s for images ...");
  vTaskDelay(pdMS_TO_TICKS(2000));
  if(got_msg)
  {
    ESP_LOGI(TAG, "Yes ... the image source is streaming ... continue the initialization");
  }
  else{
    go_to_sleep();
  }
#endif

  rtc_gpio_deinit(GAP_GPIO_IN);
  gpio_pad_select_gpio(GAP_GPIO_IN);
  gpio_set_direction(GAP_GPIO_IN, GPIO_MODE_INPUT);

  gpio_pad_select_gpio(BLINK_GPIO);
  gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
  update_led_pattern();
  xTaskCreate(led_task, "led", 4096, NULL, 4, NULL);

#ifdef CONFIG_USE_AS_AP
  ESP_LOGI(TAG, "[WiFi] Will start on AP\n");
#else
  ESP_LOGI(TAG, "[WiFi] Will connect to %s\n", CONFIG_EXAMPLE_SSID);
#endif
  wifi_init(WIFI_MODE, CONFIG_EXAMPLE_SSID, CONFIG_EXAMPLE_PASSWORD);
  xTaskCreate(wifi_task, "wifi_task", 4096, NULL, 5, NULL);

  while(1) {
    got_msg = 0;
    vTaskDelay(pdMS_TO_TICKS(2000));
#ifdef SLEEP_IF_INACTIVE
    if(!got_msg)
    {
      go_to_sleep();
    }
#endif
    source_alive = got_msg;
    update_led_pattern();
#ifdef SLEEP_IF_GPIO_LOW
    if(gpio_get_level(GAP_GPIO_IN) == 0)
    {
      go_to_sleep();
    }
#endif
  }
}
