#include "bsp/camera.h"
#include "bsp/camera/himax.h"
#include "common.h"
#include "camera.h"
#include "uart.h"
#include "io.h"
#include "led.h"
#include "nina.h"
#include "tools/frame_streamer.h"

// Apparently binning cannot be changed once the camera is running.
// [QQVGA] window is fine instead

#define         HIMAX_MODEL_ID_H          0x0000
#define         HIMAX_MODEL_ID_L          0x0001
#define         HIMAX_FRAME_COUNT         0x0005
#define         HIMAX_PIXEL_ORDER         0x0006
// R&W registers
// Sensor mode control
#define         HIMAX_MODE_SELECT         0x0100
#define         HIMAX_IMG_ORIENTATION     0x0101
#define         HIMAX_SW_RESET            0x0103
#define         HIMAX_GRP_PARAM_HOLD      0x0104
// Sensor exposure gain control
#define         HIMAX_INTEGRATION_H       0x0202
#define         HIMAX_INTEGRATION_L       0x0203
#define         HIMAX_ANALOG_GAIN         0x0205
#define         HIMAX_DIGITAL_GAIN_H      0x020E
#define         HIMAX_DIGITAL_GAIN_L      0x020F
// Frame timing control
#define         HIMAX_FRAME_LEN_LINES_H   0x0340
#define         HIMAX_FRAME_LEN_LINES_L   0x0341
#define         HIMAX_LINE_LEN_PCK_H      0x0342
#define         HIMAX_LINE_LEN_PCK_L      0x0343
// Binning mode control
#define         HIMAX_READOUT_X           0x0383
#define         HIMAX_READOUT_Y           0x0387
#define         HIMAX_BINNING_MODE        0x0390
// Test pattern control
#define         HIMAX_TEST_PATTERN_MODE   0x0601
// Black level control
#define         HIMAX_BLC_CFG             0x1000
#define         HIMAX_BLC_TGT             0x1003
#define         HIMAX_BLI_EN              0x1006
#define         HIMAX_BLC2_TGT            0x1007
//  Sensor reserved
#define         HIMAX_DPC_CTRL            0x1008
#define         HIMAX_SINGLE_THR_HOT      0x100B
#define         HIMAX_SINGLE_THR_COLD     0x100C
// VSYNC,HSYNC and pixel shift register
#define         HIMAX_VSYNC_HSYNC_PIXEL_SHIFT_EN  0x1012
// Automatic exposure gain control
#define         HIMAX_AE_CTRL             0x2100
#define         HIMAX_AE_TARGET_MEAN      0x2101
#define         HIMAX_AE_MIN_MEAN         0x2102
#define         HIMAX_CONVERGE_IN_TH      0x2103
#define         HIMAX_CONVERGE_OUT_TH     0x2104
#define         HIMAX_MAX_INTG_H          0x2105
#define         HIMAX_MAX_INTG_L          0x2106
#define         HIMAX_MIN_INTG            0x2107
#define         HIMAX_MAX_AGAIN_FULL      0x2108
#define         HIMAX_MAX_AGAIN_BIN2      0x2109
#define         HIMAX_MIN_AGAIN           0x210A
#define         HIMAX_MAX_DGAIN           0x210B
#define         HIMAX_MIN_DGAIN           0x210C
#define         HIMAX_DAMPING_FACTOR      0x210D
#define         HIMAX_FS_CTRL             0x210E
#define         HIMAX_FS_60HZ_H           0x210F
#define         HIMAX_FS_60HZ_L           0x2110
#define         HIMAX_FS_50HZ_H           0x2111
#define         HIMAX_FS_50HZ_L           0x2112
#define         HIMAX_FS_HYST_TH          0x2113
// Motion detection control
#define         HIMAX_MD_CTRL             0x2150
#define         HIMAX_I2C_CLEAR           0x2153
#define         HIMAX_WMEAN_DIFF_TH_H     0x2155
#define         HIMAX_WMEAN_DIFF_TH_M     0x2156
#define         HIMAX_WMEAN_DIFF_TH_L     0x2157
#define         HIMAX_MD_THH              0x2158
#define         HIMAX_MD_THM1             0x2159
#define         HIMAX_MD_THM2             0x215A
#define         HIMAX_MD_THL              0x215B
//  Sensor timing control
#define         HIMAX_QVGA_WIN_EN         0x3010
#define         HIMAX_SIX_BIT_MODE_EN     0x3011
#define         HIMAX_PMU_AUTOSLEEP_FRAMECNT  0x3020
#define         HIMAX_ADVANCE_VSYNC       0x3022
#define         HIMAX_ADVANCE_HSYNC       0x3023
#define         HIMAX_EARLY_GAIN          0x3035
//  IO and clock control
#define         HIMAX_BIT_CONTROL         0x3059
#define         HIMAX_OSC_CLK_DIV         0x3060
#define         HIMAX_ANA_Register_11     0x3061
#define         HIMAX_IO_DRIVE_STR        0x3062
#define         HIMAX_IO_DRIVE_STR2       0x3063
#define         HIMAX_ANA_Register_14     0x3064
#define         HIMAX_OUTPUT_PIN_STATUS_CONTROL   0x3065
#define         HIMAX_ANA_Register_17     0x3067
#define         HIMAX_PCLK_POLARITY       0x3068


#define MAX_IMAGE_SIZE (324 * 324)

static struct pi_device camera;
static int imgTransferDone = 0;
static pi_task_t task1;
static PI_L2 unsigned char *L2_image;
static int should_crop = 1;
static int camera_image_width;
static int camera_image_height;
static int cropped_image_width;
static int cropped_image_height;
static int width;
static int height;
static unsigned int image_size_bytes, input_size_bytes;
static frame_streamer_t *streamer = NULL;
static volatile int stream_done;
static pi_buffer_t buffer;

// FRAME_STREAMER_FORMAT_RAW
// FRAME_STREAMER_FORMAT_JPEG

// Let's limit to RAW streams at first, so we don't have to dealloc the jpegstreamer ...

static int set_size(camera_config_t);
static void set_format(uint8_t format);
static void set_target_value(uint8_t value);
static camera_config_t camera_config;
static camera_config_t desired_camera_config;
static volatile int _should_set_camera_config = 0;
static stream_config_t stream_config = {.on=0, .format=FRAME_STREAMER_FORMAT_RAW};
static void update_streamer();

#ifdef FIX_GAINS
static int n = 0;
#endif

//private from https://github.com/GreenWaves-Technologies/gap_sdk/blob/master/tools/gap_tools/frame_streamer/frame_streamer.c

#include "bsp/transport.h"

typedef struct
{
  struct pi_transport_header header;
  frame_streamer_open_req_t req;
} frame_streamer_open_req_full_t;


struct frame_streamer_s {
  struct pi_device *transport;
  frame_streamer_format_e format;
  int channel;
  struct pi_transport_header header;
  frame_streamer_open_req_full_t req;
  void *jpeg;
  unsigned int height;
  unsigned int width;
};


int init_streamer(struct pi_device *transport, uint8_t format)
{
  struct frame_streamer_conf frame_streamer_conf;
  frame_streamer_conf_init(&frame_streamer_conf);
  frame_streamer_conf.transport = transport;
  frame_streamer_conf.format = format;
  frame_streamer_conf.width = width;
  frame_streamer_conf.height = height;
  frame_streamer_conf.depth = 1;
  frame_streamer_conf.name = "camera";
  streamer = frame_streamer_open(&frame_streamer_conf);
  if (streamer){
    pi_buffer_init(&buffer, PI_BUFFER_TYPE_L2, L2_image);
    update_streamer();
    return 0;
  }
  LOG("Failed to open streamer\n");
  return -1;
}


void set_stream_config(stream_config_t *config)
{
  if(stream_config.on && !config->on)
  {
    stream_config.on = 0;
    // put_nina_to_sleep();
  }
  else if(!stream_config.on && config->on)
  {
    if(!streamer)
    {
      struct pi_device *transport = NULL;
      if(config->transport == TRANSPORT_WIFI)
      {
        // wake_up_nina();
        transport = open_wifi();
      }
      else if(config->transport == TRANSPORT_PIPE)
      {
        transport = open_pipe("/tmp/image_pipe");
      }
      if(transport && init_streamer(transport, config->format) == 0)
      {
        stream_config.on = 1;
      }
    }
    else{
      stream_config.on = 1;
    }
  }
  // Not allowed to change format of an initialized streamer
  if(!streamer)
  {
    if(config->format < 2)
      stream_config.format = config->format;
    if(config->transport < 2)
      stream_config.transport = config->transport;
  }
  send_stream_config(&stream_config);
  LOG("Has set stream to on = %d, format = %d, transport = %d\n",
      stream_config.on, stream_config.format, stream_config.transport);
}

void set_camera_config(camera_config_t *_config)
{
  desired_camera_config = *_config;
  _should_set_camera_config = 1;
}

int _set_camera_config(camera_config_t *_config)
{
  int error = 0;
  if(streamer && stream_config.format == FRAME_STREAMER_FORMAT_JPEG)
  {
    // Not allowed to change buffer size!
  }
  else
  {
    error = set_size(*_config);
    if(!error)
    {
      camera_config.top = _config->top;
      camera_config.bottom = _config->bottom;
      camera_config.right = _config->right;
      camera_config.left = _config->left;
      camera_config.step = _config->step;
    }
  }
  if(!error && camera_config.format != _config->format &&  _config->format >=0 && _config->format < 4)
  {
    set_format(_config->format);
    camera_config.format = _config->format;
  }
  if(!error && camera_config.target_value != _config->target_value)
  {
    set_target_value(_config->target_value);
    camera_config.target_value = _config->target_value;
  }
  // camera_config = *_config;
  update_streamer();
  send_camera_config(&camera_config);
  LOG("Has set camera config to margin = [%d %d %d %d], step = %d, format = %d, target = %d\n",
      camera_config.top, camera_config.right, camera_config.bottom, camera_config.left,
      camera_config.step, camera_config.format, camera_config.target_value);
  if(error)
  {
    uart_flush_rx();
  }
  return error;
}

static void update_streamer()
{
  pi_buffer_set_format(&buffer, width, height, 1, PI_BUFFER_FORMAT_GRAY);
  // LOG("Set buffer %d x %d = %d\n", buffer.width, buffer.height, pi_buffer_size(&buffer));
  // I cannot, as the struct is private! but maybe I don't need it, at least for RAW
  streamer->width = width;
  streamer->height = height;
  if(stream_config.format == FRAME_STREAMER_FORMAT_JPEG)
    return;
  // Let us use info to tell NINA the width and color format of the frame!
  // 0: GS, 1: BG, 2: GB, 3: GR, 4: RG
  uint32_t color = 0; // Grazyscale
  if((camera_config.format == FULL || camera_config.format == QVGA) && camera_config.step == 1)
  {
    // BAYER
    color = (camera_config.top % 2 << 1) + (camera_config.left % 2) + 1;
  }
  // bytes 1-2: width, bytes 3-4: color
  streamer->header.info = width + (color << 16);
}

static int32_t pi_camera_reg_get16(struct pi_device *device, uint32_t reg_addr_l, uint32_t reg_addr_h, uint16_t *value)
{
  uint8_t *v = (uint8_t *) value;
  pi_camera_reg_get(&camera, reg_addr_l, v);
  return pi_camera_reg_get(&camera, reg_addr_h, v+1);
}

static int32_t pi_camera_reg_set16(struct pi_device *device, uint32_t reg_addr_l, uint32_t reg_addr_h, uint16_t *value)
{
  uint8_t *v = (uint8_t *) value;
  pi_camera_reg_set(&camera, reg_addr_l, v);
  return pi_camera_reg_get(&camera, reg_addr_h, v+1);
}

void static fix_gains();
static void enqueue_capture();

void start_camera_loop()
{
  enqueue_capture(NULL);
}

static void streamer_handler(void *task)
{
  if(task)
  {
    pi_task_push((pi_task_t *)task);
  }
  else{
    enqueue_capture(NULL);
  }
}

static void end_of_frame(void *task) {

  pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);
  if(_should_set_camera_config)
  {
    // set_format(config.format);
    _set_camera_config(&desired_camera_config);
    _should_set_camera_config = 0;
  }

  // LOG("New frame %03d %03d %03d %03d ...\n", L2_image[0], L2_image[1], L2_image[2], L2_image[3]);

  toggle_led();
  #ifdef FIX_GAINS
  fix_gains();
  #endif

  if(should_crop)
  {
    unsigned char * origin     = (unsigned char *) L2_image;
    unsigned char * ptr_crop   = (unsigned char *) L2_image;
    int init_offset = camera_image_width * camera_config.top + camera_config.left;
    int outid = 0;
    int step = camera_config.step;
    for(int i=0; i<cropped_image_height; i+=step)
    {
      // rt_event_execute(NULL, 0);
      unsigned char * line = ptr_crop + init_offset + camera_image_width * i;
      for(int j=0; j<cropped_image_width; j+=step)
      {
        origin[outid] = line[j];
        outid++;
      }
    }
  }

  if(stream_config.on)
  {
    stream_done = 0;
    // int size = pi_buffer_size(&buffer);
    frame_streamer_send_async(streamer, &buffer, pi_task_callback(&task1, streamer_handler, task));
  }
  else if(task)
  {
    pi_task_push((pi_task_t *)task);
  }
  else{
    enqueue_capture(NULL);
  }
}


static void enqueue_capture(pi_task_t * task) {
  pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
  pi_camera_capture_async(&camera, (unsigned char*)L2_image, image_size_bytes, pi_task_callback(&task1, end_of_frame, task));
}

unsigned char* grab_camera_frame()
{
  pi_task_t task;
  enqueue_capture(pi_task_block(&task));
  pi_task_wait_on(&task);
  return L2_image;
}

static void set_format(uint8_t format)
{
  uint8_t value;
  if(format == QQVGA || format == HALF)
  {
    value=0x03;
  }
  else
  {
    value = 0x01;
  }
  pi_camera_reg_set(&camera, HIMAX_READOUT_X, &value);
  pi_camera_reg_set(&camera, HIMAX_READOUT_Y, &value);
  pi_camera_reg_set(&camera, HIMAX_BINNING_MODE, &value);

  // pi_camera_reg_get(&camera, HIMAX_READOUT_X, &value);
  // LOG("HIMAX_READOUT_X %d\n", value);
  // pi_camera_reg_get(&camera, HIMAX_READOUT_Y, &value);
  // LOG("HIMAX_READOUT_Y %d\n", value);
  // pi_camera_reg_get(&camera, HIMAX_BINNING_MODE, &value);
  // LOG("HIMAX_BINNING_MODE %d\n", value);

  if(format == QQVGA || format == QVGA)
  {
    value=0x01;
  }
  else{
    value=0x00;
  }
  pi_camera_reg_set(&camera, HIMAX_QVGA_WIN_EN, &value);

  #ifdef FIX_GAINS
  n = 0;
  #endif
}

static void set_target_value(uint8_t value)
{
  uint8_t s_value = 0x00;
  pi_camera_reg_set(&camera, HIMAX_AE_CTRL, &s_value);
  pi_camera_reg_set(&camera, HIMAX_AE_TARGET_MEAN, &value);
  #ifdef FIX_GAINS
  n = 0;
  #endif
}

static void configure_camera_registers()
{
  // LOG("Configuring the camera\n");
  uint8_t set_value = 3;
  // uint8_t reg_value;
  pi_camera_reg_set(&camera, IMG_ORIENTATION, &set_value);
  // pi_camera_reg_get(&camera, IMG_ORIENTATION, &reg_value);
  // LOG("img orientation %d\n",reg_value);

  // set_format(format);
  // set_target_value(target_value);

  // set_value=0x03;
  // pi_camera_reg_set(&camera, HIMAX_MAX_AGAIN_BIN2, &set_value);
  // pi_camera_reg_set(&camera, HIMAX_MAX_AGAIN_FULL, &set_value);
  //
  // set_value=0x00;
  // pi_camera_reg_set(&camera, HIMAX_FS_CTRL, &set_value);

}

static int set_size(camera_config_t c)
{
  switch(c.format){
    case QVGA:
      camera_image_width = 324;
      camera_image_height = 244;
      break;
    case QQVGA:
      camera_image_width = 162;
      camera_image_height = 122;
      break;
    case HALF:
      camera_image_width = 162;
      camera_image_height = 162;
      break;
    case FULL:
      camera_image_width = 324;
      camera_image_height = 324;
      break;
    default:
      LOG_ERROR("Unknown format %d\n", c.format);
      return 1;
  }

  if(c.top < 0 || c.bottom <0 || (c.top + c.bottom) >  camera_image_height)
  {
    LOG_ERROR("Wrong image size top %d, bottom %d, height = %d\n",
        c.top, c.bottom, camera_image_height);
    return 1;
  }
  if(c.left < 0 || c.right <0 || (c.left + c.right) >  camera_image_width)
  {
    LOG_ERROR("Wrong image size left %d, right %d, width = %d\n",
        c.left, c.right, camera_image_width);
    return 1;
  }
  if(c.step <= 0 || c.step >= camera_image_width || c.step >= camera_image_height)
  {
    LOG_ERROR("Wrong step size %d, width = %d, height = %d\n",
        c.step, camera_image_width, camera_image_height);
    return 1;
  }
  cropped_image_width = camera_image_width - c.left - c.right;
  cropped_image_height = camera_image_height - c.bottom - c.top;
  should_crop = c.left || c.right || c.bottom || c.top;
  width = cropped_image_width / c.step;
  height = cropped_image_height / c.step;
  // input_size_bytes = CAM_DSMPL_W * CAM_DSMPL_H * sizeof(unsigned char);
  image_size_bytes = camera_image_width * camera_image_height * sizeof(unsigned char);
  input_size_bytes = width * height * sizeof(unsigned char);
  LOG("Will capture %d x %d frames, crop (%d) with margins of %d %d %d %d pixels, "
      "sample every %d pixels, to finally get %d x %d frames\n",
       camera_image_width, camera_image_height, should_crop, c.top, c.right, c.bottom, c.left,
       c.step, width, height);
  return 0;
}

int init_camera(camera_config_t _camera_config, stream_config_t _stream_config)
{
  struct pi_himax_conf cam_conf;
  pi_himax_conf_init(&cam_conf);

  cam_conf.format = PI_CAMERA_QVGA;

  pi_open_from_conf(&camera, &cam_conf);

  if (pi_camera_open(&camera))
  {
    LOG_ERROR("Failed to open Himax camera\n");
    return -1;
  }

  if(_set_camera_config(&_camera_config))
    return -1;
  configure_camera_registers();

  // wait the camera to setup
  // if(rt_platform() == ARCHI_PLATFORM_BOARD)
  #ifndef GVSOC
  pi_time_wait_us(1000000);
  #endif

  // allocate the memory of L2 for the image buffer
  L2_image = pi_l2_malloc(MAX_IMAGE_SIZE);
  if (L2_image == NULL){
    LOG_ERROR("Failed to alloc L2 memeory of size %d\n", image_size_bytes)
    return -1;
  }
  // LOG("L2 Image alloc\t%dB\t@ 0x%08x:\t%s", image_size_bytes, (unsigned int) L2_image, L2_image?"Ok":"Failed");

  set_stream_config(&_stream_config);
  LOG("Initialized Himax camera\n");
  return 0;
}

void close_camera()
{
  //TODO (Jerome, GAPSDK): should free streamer
  pi_camera_close(&camera);
  pi_l2_free(L2_image, image_size_bytes);
  LOG("Closed camera\n");
}

#ifdef FIX_GAINS
static uint8_t a_gain[FIX_GAINS];
static uint16_t integration[FIX_GAINS];
static uint16_t d_gain[FIX_GAINS];

static uint16_t median16(int n, uint16_t x[]) {
    uint16_t temp;
    int i, j;
    for(i=0; i<n-1; i++) {
        for(j=i+1; j<n; j++) {
            if(x[j] < x[i]) {
                // swap elements
                temp = x[i];
                x[i] = x[j];
                x[j] = temp;
            }
        }
    }
    return x[n/2];
}

static uint8_t median8(int n, uint8_t x[]) {
    uint8_t temp;
    int i, j;
    for(i=0; i<n-1; i++) {
        for(j=i+1; j<n; j++) {
            if(x[j] < x[i]) {
                // swap elements
                temp = x[i];
                x[i] = x[j];
                x[j] = temp;
            }
        }
    }
    return x[n/2];
}

static void fix_gains()
{
  if(n>=FIX_GAINS)
  {
    #ifdef DEBUG_AE
    uint8_t value8;
    uint16_t value16;
    pi_camera_reg_get16(&camera, HIMAX_INTEGRATION_L, HIMAX_INTEGRATION_H, &value16);
    LOG("integration time: %d\n", value16);
    pi_camera_reg_get(&camera, HIMAX_ANALOG_GAIN, &value8);
    LOG("analog gain: %d\n", value8);
    pi_camera_reg_get16(&camera, HIMAX_DIGITAL_GAIN_L, HIMAX_DIGITAL_GAIN_H, &value16);
    LOG("digital gain %d\n", value16);
    #endif
    return;
  }

  // Force the control to be on
  uint8_t value8;
  if(n < 2)
  {
    value8 = 0x01;
    pi_camera_reg_set(&camera, HIMAX_AE_CTRL, &value8);
  }

  pi_camera_reg_get16(&camera, HIMAX_INTEGRATION_L, HIMAX_INTEGRATION_H, integration+n);
  pi_camera_reg_get(&camera, HIMAX_ANALOG_GAIN, a_gain+n);
  pi_camera_reg_get16(&camera, HIMAX_DIGITAL_GAIN_L, HIMAX_DIGITAL_GAIN_H, d_gain+n);

  n++;

  if(n==FIX_GAINS)
  {
    uint16_t m_integration = median16(FIX_GAINS, integration);
    uint16_t m_dgain = median16(FIX_GAINS, d_gain);
    uint8_t m_again = median8(FIX_GAINS, a_gain);

    #ifdef DEBUG_AE
    LOG("---- AE Cumulated values ----\n");
    LOG("integration time:");
    for (size_t i = 0; i < FIX_GAINS; i++) {
      LOG("%d, ", integration[i]);
    }
    LOG("=> median %d \n", m_integration);
    LOG("analog gain: ");
    for (size_t i = 0; i < FIX_GAINS; i++) {
      LOG("%d, ", a_gain[i]);
    }
    LOG("=> median %d\n", m_again);
    LOG("digital gain: ");
    for (size_t i = 0; i < FIX_GAINS; i++) {
      LOG("%d, ", d_gain[i]);
    }
    LOG("=> median %d\n", m_dgain);
    LOG("-----------------------------\n");
    #endif

    LOG("Disable AE control after %d steps and fix registers:\n"
        "\t\t- INTEGRATION:  %d\n"
        "\t\t- DIGITAL GAIN: %d\n"
        "\t\t- ANALOG GAIN:  %d\n", FIX_GAINS, m_integration, m_dgain, m_again);

    uint8_t value8 = 0x00;
    pi_camera_reg_set(&camera, HIMAX_AE_CTRL, &value8);
    pi_camera_reg_set16(&camera, HIMAX_INTEGRATION_L, HIMAX_INTEGRATION_H, &m_integration);
    pi_camera_reg_set16(&camera, HIMAX_DIGITAL_GAIN_L, HIMAX_DIGITAL_GAIN_H, &m_dgain);
    pi_camera_reg_set(&camera, HIMAX_ANALOG_GAIN, &m_again);
  }
}

#endif
