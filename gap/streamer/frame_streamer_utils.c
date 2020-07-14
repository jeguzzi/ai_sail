#include "frame_streamer_utils.h"
#include "uart_protocol.h"
#include "common.h"

//private from https://github.com/GreenWaves-Technologies/gap_sdk/blob/master/tools/gap_tools/frame_streamer/frame_streamer.c

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

frame_streamer_t * init_streamer(struct pi_device *transport, uint8_t format,
                                 uint16_t width, uint16_t height)
{
  struct frame_streamer_conf frame_streamer_conf;
  frame_streamer_conf_init(&frame_streamer_conf);
  frame_streamer_conf.transport = transport;
  frame_streamer_conf.format = format;
  frame_streamer_conf.width = width;
  frame_streamer_conf.height = height;
  frame_streamer_conf.depth = 1;
  frame_streamer_conf.name = "camera";
  frame_streamer_t * streamer = frame_streamer_open(&frame_streamer_conf);
  if (!streamer){
    LOG_ERROR("Failed to open streamer\n");
  }
  return streamer;
}

void set_streamer(frame_streamer_t * streamer, uint16_t width, uint16_t height, int color)
{
  streamer->width = width;
  streamer->height = height;
  if(streamer->format == FRAME_STREAMER_FORMAT_JPEG)
    return;
  // We use info to tell NINA the width and color format of the frame!
  // bytes 1-2: width, bytes 3-4: color
  streamer->header.info = width + (color << 16);
}
