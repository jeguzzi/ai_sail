/**
 * This header defines a couple of convenient functions to work with frame streamers
 * from  "tools/frame_streamer.h". In particular, they let the user update the information
 * encoded in the header of RAW images' [size and color].
 *
 * Author: Jérôme Guzzi (jerome@idsia.ch)
 * License: MIT
 */

#ifndef __FRAME_STREAMER_UTILS_H__
#define __FRAME_STREAMER_UTILS_H__

#include "tools/frame_streamer.h"
#include "bsp/transport.h"

/**
 * Initialize a frame streamer.
 * @param  transport The transport that can be any object that conform to the trasnport protocol
 *                   (in our case wifi or pipe).
 * @param  format    FRAME_STREAMER_FORMAT_RAW = 0 for RAW and FRAME_STREAMER_FORMAT_JPEG = 1 for JPEG
 * @param  width     The width of the frame
 * @param  height    The height of the frame
 * @return           A poiter to the allocated object or NULL if initialization failed.
 */
frame_streamer_t * init_streamer(struct pi_device *transport, uint8_t format,
                                 uint16_t width, uint16_t height);

/**
 * Update the streamer config (only helpful for RAW format, for which [width, height, color]
 * are encoded in the frame header
 * @param streamer The streamer
 * @param width    The width of the frame
 * @param height   The height of the frame
 * @param color    The color: 0 for GS, 1 for BG, 2 for GB, 3 for GR, 4 for RG
 */
void set_streamer(frame_streamer_t * streamer,
                  uint16_t width, uint16_t height, int color);

#endif /* end of include guard: __FRAME_STREAMER_UTILS_H__ */
