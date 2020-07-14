/*
This file is an extension of the original example provided by Bitcraze
 */
#define DEBUG_MODULE "AIDECK"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "stm32fxxx.h"
#include "config.h"
#include "console.h"
#include "uart1.h"
#include "debug.h"
#include "deck.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "log.h"
#include "param.h"
#include "system.h"
#include "uart1.h"
#include "uart2.h"
#include "timers.h"
#include "worker.h"

// Include the autogenerate code implementing the protocol
#include "aideck_protocol.c"

#ifdef AIDECK_HAS_CONFIGS
static xTimerHandle timer;
static void configTimer(xTimerHandle timer)
{
  workerSchedule(update_config, NULL);
}
#endif

static bool isInit = false;

// Uncomment when NINA printout read is desired from console
// #define DEBUG_NINA_PRINT

#ifdef DEBUG_NINA_PRINT
static void NinaTask(void *param)
{
    systemWaitStart();
    vTaskDelay(M2T(1000));
    DEBUG_PRINT("Starting reading out NINA debugging messages:\n");
    vTaskDelay(M2T(2000));

    // Pull the reset button to get a clean read out of the data
    pinMode(DECK_GPIO_IO4, OUTPUT);
    digitalWrite(DECK_GPIO_IO4, LOW);
    vTaskDelay(10);
    digitalWrite(DECK_GPIO_IO4, HIGH);
    pinMode(DECK_GPIO_IO4, INPUT_PULLUP);

    // Read out the byte the NINA sends and immediately send it to the console.
    uint8_t byte;
    while (1)
    {
        if (uart2GetDataWithTimout(&byte) == true)
        {
            consolePutchar(byte);
        }
        if(in_byte != old_byte)
        {
          old_byte = in_byte;
          uart2SendData(1, &old_byte);
        }
    }
}
#endif

// Read n bytes from UART, returning the read size before ev. timing out.
static int read_uart_bytes(int size, uint8_t *buffer)
{
  uint8_t *byte = buffer;
  for (int i = 0; i < size; i++) {
    if(uart1GetDataWithTimout(byte))
    {
      byte++;
    }
    else
    {
      return i;
    }
  }
  return size;
}

// Read UART 1 while looking for structured messages.
// When none are found, print everything to console.
static uint8_t header_buffer[HEADER_LENGTH];
static uint8_t rx_buffer[BUFFER_LENGTH];

static void read_uart_message()
{
  uint8_t *byte = header_buffer;
  int n = 0;
  input_t *input;
  input_t *begin = (input_t *) inputs;
  input_t *end = begin + INPUT_NUMBER;
  for (input = begin; input < end; input++) input->valid = 1;
  while(n < HEADER_LENGTH)
  {
    if(uart1GetDataWithTimout(byte))
    {
      int valid = 0;
      for (input = begin; input < end; input++) {
        if(!(input->valid)) continue;
        if(*byte != (input->header)[n]){
          input->valid = 0;
        }
        else{
          valid = 1;
        }
      }
      n++;
      if(valid)
      {
        // Keep reading
        byte++;
        continue;
      }
    }
    // forward to console and return;
    for (size_t i = 0; i < n; i++) {
      consolePutchar(header_buffer[i]);
    }
    return;
  }
  // Found message
  for (input = begin; input < end; input++)
  {
    if(input->valid) break;
  }
  int size = read_uart_bytes(input->size, rx_buffer);
  if( size == input->size )
  {
    // Call the corresponding callback
    input->callback(rx_buffer);
  }
  else{
    DEBUG_PRINT("Failed to receive message %4s: (%d vs %d bytes received)\n",
                 input->header, size, input->size);
  }
}

static void Gap8Task(void *param)
{
    systemWaitStart();
    vTaskDelay(M2T(1000));

    // Pull the reset button to get a clean read out of the data
    pinMode(DECK_GPIO_IO4, OUTPUT);
    digitalWrite(DECK_GPIO_IO4, LOW);
    vTaskDelay(10);
    digitalWrite(DECK_GPIO_IO4, HIGH);
    pinMode(DECK_GPIO_IO4, INPUT_PULLUP);
    DEBUG_PRINT("Starting UART listener\n");
    while (1)
    {
        read_uart_message();
    }
}

static void aideckInit(DeckInfo *info)
{
    if (isInit)
        return;

    // Intialize the UART for the GAP8
    uart1Init(115200);
    // Initialize task for the GAP8
    xTaskCreate(Gap8Task, AI_DECK_GAP_TASK_NAME, AI_DECK_TASK_STACKSIZE, NULL,
                AI_DECK_TASK_PRI, NULL);

#ifdef DEBUG_NINA_PRINT
    // Initialize the UART for the NINA
    uart2Init(115200);
    // Initialize task for the NINA
    xTaskCreate(NinaTask, AI_DECK_NINA_TASK_NAME, AI_DECK_TASK_STACKSIZE, NULL,
                AI_DECK_TASK_PRI, NULL);
#endif
#ifdef AIDECK_HAS_CONFIGS
  timer = xTimerCreate( "configTimer", M2T(1000), pdTRUE, NULL, configTimer );
  xTimerStart(timer, 1000);
#endif
  isInit = true;
}

static bool aideckTest()
{
    return true;
}

static const DeckDriver aideck_deck = {
    .vid = 0xBC,
    .pid = 0x12,
    .name = "bcAI",

    .usedPeriph = 0,
    .usedGpio = 0, // FIXME: Edit the used GPIOs

    .init = aideckInit,
    .test = aideckTest,
};

PARAM_GROUP_START(deck)
PARAM_ADD(PARAM_UINT8 | PARAM_RONLY, bcAIDeck, &isInit)
PARAM_GROUP_STOP(deck)

DECK_DRIVER(aideck_deck);
