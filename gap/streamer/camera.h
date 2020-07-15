#ifndef __CAMERA_H__
#define __CAMERA_H__

/**
 * This header defines the main interaction with the camera
 * - initialize
 * - start the [camera] run-loop
 * - clean up
 * The interfaces to dynamicaly configure the camera are defined in uart_protocol.h instead.
 * Author: Jérôme Guzzi (jerome@idsia.ch)
 * License: MIT
 */

#include <stdbool.h>

#include "pmsis.h"
#include "frame_streamer_utils.h"
#include "uart_protocol.h"
#include "himax_utils.h"

#define DYNAMIC_IMAGE_SIZE
#define DYNAMIC_CAMERA_SIZE

typedef struct{
  struct pi_device *device;
  int camera_width;
  int camera_height;
  int cropped_width;
  int cropped_height;
  int width;
  int height;
  int camera_size;
  int size;
  bool should_crop;
  bool initialized;
  camera_config_t config;
} image_acquisition_t;

typedef struct{
  frame_streamer_t *streamer;
  pi_buffer_t buffer;
  stream_config_t config;
} image_streaming_t;

#define default_camera_config(...) ((camera_config_t){.step=1, .ae=1, .target_value=80, .fps=20, .format=HALF, .bottom=1, .top=1, __VA_ARGS__})
#define default_stream_config(...) ((stream_config_t){.transport = TRANSPORT_WIFI, __VA_ARGS__})
#define FIX_EXPOSURE_AFTER 10
#define TRANSPORT_WIFI 0
// #define TRANSPORT_PIPE 1
#define LED_PERIOD 10

/**
 * Initialize the camera: open and configure the himax, if needed create and confingure the streamer.
 *                        It does not allocate a buffer to store the image camera (this is postponed until the camera loop is started).
 * @param  camera_config_t The camera configuration
 * @param  stream_config_t The streaming configuration
 * @return                 A value differnt if initialization failed.
 */
int init_camera(camera_config_t, stream_config_t);
void close_camera();
/**
 * Allocate the camera buffeer and start the camera loop: grabbing - streaming - serving
 * @param wait       Defines if the loop should wait for a client process to unlock the frame before grabbing a new one
 * @param concurrent Defines if streaming and serving are concurrent processes.
 *                   If false, it will wait until streaming is finished before serving the image.
 */
void start_camera_loop(bool wait, bool concurrent);

typedef struct {
  unsigned char * image;
  pi_task_t * release_task;
} image_event_t;

/**
 * The main method to get a new image for a client running in a different event loop.
 * It will block [yield] until a new image is available.
 * @return An image_event_t struct with the image and the task (which acts as a reentrant lock)
 *         that must be pushed as soon as possible by the client to free the frame.
 */
image_event_t wait_for_new_image();

#endif // __CAMERA_H__
