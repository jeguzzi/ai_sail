#ifndef __FRAME_STREAMER_UTILS_H__
#define __FRAME_STREAMER_UTILS_H__

#include "tools/frame_streamer.h"
#include "bsp/transport.h"

// FRAME_STREAMER_FORMAT_RAW
// FRAME_STREAMER_FORMAT_JPEG

frame_streamer_t * init_streamer(struct pi_device *transport, uint8_t format,
                                 uint16_t width, uint16_t height);

// colors -- 0: GS, 1: BG, 2: GB, 3: GR, 4: RG -- only relevant to RAW
void set_streamer(frame_streamer_t * streamer,
                  uint16_t width, uint16_t height, int color);

#endif /* end of include guard: __FRAME_STREAMER_UTILS_H__ */
