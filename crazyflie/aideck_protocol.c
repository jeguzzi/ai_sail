/*
  This file was autogenerate on 07/14/20 from the following Python config:

  from base import config, c_uint8, c_uint16  # type: ignore


  @config(name='camera', group='aideck_HIMAX', header="!CAM")
  class Camera:
      marginTop: c_uint16
      marginRight: c_uint16
      marginBottom: c_uint16
      marginLeft: c_uint16
      format: c_uint8  # noqa
      step: c_uint8
      target_value: c_uint8
      ae: c_uint8
      fps: c_uint8


  @config(name='stream', group='aideck_stream', header="!STR")
  class Stream:
      on: c_uint8
      format: c_uint8  # noqa
      transport: c_uint8

*/

#define HEADER_LENGTH 4
#define BUFFER_LENGTH 13
#define VERBOSE_RX
//#define VERBOSE_TX
#define REQUEST_TIMEOUT 2000 // number of milliseconds to wait for a confirmation
#define INPUT_NUMBER 2
#define AIDECK_HAS_CONFIGS



typedef struct {
  const char *header;
  uint8_t size;
  void (*callback)(void *);
  bool valid;
} input_t;
// --- config camera

typedef struct {
  uint16_t marginTop;
  uint16_t marginRight;
  uint16_t marginBottom;
  uint16_t marginLeft;
  uint8_t format;
  uint8_t step;
  uint8_t target_value;
  uint8_t ae;
  uint8_t fps;
} __attribute__((packed)) camera_t;


void log_camera(camera_t *value)
{
  DEBUG_PRINT("marginTop=%u, marginRight=%u, marginBottom=%u, marginLeft=%u, format=%u, step=%u, target_value=%u, ae=%u, fps=%u\n", value->marginTop,value->marginRight,value->marginBottom,value->marginLeft,value->format,value->step,value->target_value,value->ae,value->fps);
}

static struct {
  uint32_t request_time;
  camera_t value, dvalue;
  const char *header;
} __camera__config = {.request_time=0, .header= "!CAM" };

static void __camera_cb(void *buffer)
{
  camera_t *value = (camera_t *)buffer;
  __camera__config.value = *value;
#ifdef VERBOSE_RX
  uint32_t now = T2M(xTaskGetTickCount());
  if(__camera__config.request_time && memcmp(&(__camera__config.value), &(__camera__config.dvalue), sizeof(camera_t)))
  {
    DEBUG_PRINT("[WARNING] GAP has set camera config after %ld ms\n", now - __camera__config.request_time);
    log_camera(&(__camera__config.value));
    DEBUG_PRINT("to a different value than the one requested\n");
    log_camera(&(__camera__config.dvalue));
  }
  else{
    if(__camera__config.request_time)
      DEBUG_PRINT("GAP has set camera config after %ld ms to\n", now - __camera__config.request_time);
    else
      DEBUG_PRINT("GAP has set camera config to\n");
    log_camera(&(__camera__config.value));
  }
#endif // VERBOSE_RX
  __camera__config.dvalue = __camera__config.value;
  __camera__config.request_time = 0;
}

// --- config stream

typedef struct {
  uint8_t on;
  uint8_t format;
  uint8_t transport;
} __attribute__((packed)) stream_t;


void log_stream(stream_t *value)
{
  DEBUG_PRINT("on=%u, format=%u, transport=%u\n", value->on,value->format,value->transport);
}

static struct {
  uint32_t request_time;
  stream_t value, dvalue;
  const char *header;
} __stream__config = {.request_time=0, .header= "!STR" };

static void __stream_cb(void *buffer)
{
  stream_t *value = (stream_t *)buffer;
  __stream__config.value = *value;
#ifdef VERBOSE_RX
  uint32_t now = T2M(xTaskGetTickCount());
  if(__stream__config.request_time && memcmp(&(__stream__config.value), &(__stream__config.dvalue), sizeof(stream_t)))
  {
    DEBUG_PRINT("[WARNING] GAP has set stream config after %ld ms\n", now - __stream__config.request_time);
    log_stream(&(__stream__config.value));
    DEBUG_PRINT("to a different value than the one requested\n");
    log_stream(&(__stream__config.dvalue));
  }
  else{
    if(__stream__config.request_time)
      DEBUG_PRINT("GAP has set stream config after %ld ms to\n", now - __stream__config.request_time);
    else
      DEBUG_PRINT("GAP has set stream config to\n");
    log_stream(&(__stream__config.value));
  }
#endif // VERBOSE_RX
  __stream__config.dvalue = __stream__config.value;
  __stream__config.request_time = 0;
}


static input_t inputs[INPUT_NUMBER]  = {
  { .header = "!CAM", .callback = __camera_cb, .size = sizeof(camera_t) },
  { .header = "!STR", .callback = __stream_cb, .size = sizeof(stream_t) }
};




void update_config(void *data)
{
uint32_t now = T2M(xTaskGetTickCount());
  if(__camera__config.request_time)
  {
    if(now - __camera__config.request_time < REQUEST_TIMEOUT) return;
  #ifdef VERBOSE_RX
    DEBUG_PRINT("[WARNING] Request to GAP to update camera config has timed out after %ld ms\n", now - __camera__config.request_time);
  #endif // VERBOSE_RX
    __camera__config.request_time = 0;
  }
  if(memcmp(&(__camera__config.value), &(__camera__config.dvalue), sizeof(camera_t)))
  {
  #ifdef VERBOSE_TX
    DEBUG_PRINT("Will request GAP to update camera config\n");
    log_camera(&(__camera__config.value));
  #endif // VERBOSE_TX
    __camera__config.request_time = T2M(xTaskGetTickCount());
    uart1SendData(HEADER_LENGTH, (uint8_t *) __camera__config.header);
    uart1SendData(sizeof(camera_t), (uint8_t *)&(__camera__config.value));
    __camera__config.dvalue = __camera__config.value;
    return;
  }

  if(__stream__config.request_time)
  {
    if(now - __stream__config.request_time < REQUEST_TIMEOUT) return;
  #ifdef VERBOSE_RX
    DEBUG_PRINT("[WARNING] Request to GAP to update stream config has timed out after %ld ms\n", now - __stream__config.request_time);
  #endif // VERBOSE_RX
    __stream__config.request_time = 0;
  }
  if(memcmp(&(__stream__config.value), &(__stream__config.dvalue), sizeof(stream_t)))
  {
  #ifdef VERBOSE_TX
    DEBUG_PRINT("Will request GAP to update stream config\n");
    log_stream(&(__stream__config.value));
  #endif // VERBOSE_TX
    __stream__config.request_time = T2M(xTaskGetTickCount());
    uart1SendData(HEADER_LENGTH, (uint8_t *) __stream__config.header);
    uart1SendData(sizeof(stream_t), (uint8_t *)&(__stream__config.value));
    __stream__config.dvalue = __stream__config.value;
    return;
  }

}


PARAM_GROUP_START(aideck_HIMAX)
PARAM_ADD(PARAM_UINT16, marginTop, &(__camera__config.value.marginTop))
PARAM_ADD(PARAM_UINT16, marginRight, &(__camera__config.value.marginRight))
PARAM_ADD(PARAM_UINT16, marginBottom, &(__camera__config.value.marginBottom))
PARAM_ADD(PARAM_UINT16, marginLeft, &(__camera__config.value.marginLeft))
PARAM_ADD(PARAM_UINT8, format, &(__camera__config.value.format))
PARAM_ADD(PARAM_UINT8, step, &(__camera__config.value.step))
PARAM_ADD(PARAM_UINT8, target_value, &(__camera__config.value.target_value))
PARAM_ADD(PARAM_UINT8, ae, &(__camera__config.value.ae))
PARAM_ADD(PARAM_UINT8, fps, &(__camera__config.value.fps))
PARAM_GROUP_STOP(aideck_HIMAX)

PARAM_GROUP_START(aideck_stream)
PARAM_ADD(PARAM_UINT8, on, &(__stream__config.value.on))
PARAM_ADD(PARAM_UINT8, format, &(__stream__config.value.format))
PARAM_ADD(PARAM_UINT8, transport, &(__stream__config.value.transport))
PARAM_GROUP_STOP(aideck_stream)

