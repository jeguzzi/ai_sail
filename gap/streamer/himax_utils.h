/**
 * This header provides an higher-level (wrt. setting registers) interface with an himax camera.
 *
 * Author: Jérôme Guzzi (jerome@idsia.ch)
 * License: MIT
 */

// TODO (Jerome): set larger values for maximal integration and maximal a/d gains.

#ifndef __CAMERA_UTILS_H__
#define __CAMERA_UTILS_H__

#include "pmsis.h"

/**
 * Open the camera
 * @param  camera The camera
 * @return        A value different than zero in case of failure.
 */
int himax_open(struct pi_device *camera);

/**
 * Close the camera
 * @param camera The camera
 */
void himax_close(struct pi_device *camera);

/**
* The 4 camera acquistion formats
*/
#define QVGA 0   // 324 x 244
#define QQVGA 1  // 162 x 122
#define FULL 2   // 324 x 324
#define HALF 3   // 162 x 162

/**
 * Set the camera acquisition formar
 * @param camera The camera
 * @param format The format: 0 -> QVGA, 1 -> QQVGA, 2 -> FULL (324p), 3, HALF (162p)
 */
void himax_set_format(struct pi_device * camera, uint8_t format);

/**
 * Set the camera target value
 * @param camera The camera
 * @param value  The mean target intesity over all pixels.
 */
void himax_set_target_value(struct pi_device * camera, uint8_t value);

/**
 * Set the camera frame rate. It set a minimal line length while picking the number of lines
 * to meet the desired fps. In this way, the acquisition time is mimimal.
 * @param  camera The camera
 * @param  fps    The desired frame rate [frames per second]
 * @param  format The format (need to select the appropriate number of lines)
 * @return        The actual frame rate chosen, as the nearest feasible value.
 */
uint8_t himax_set_fps(struct pi_device * camera, uint8_t fps, uint8_t format);

/**
 * Set the default configuration for the camera.
 * At the moment, it just set the correct orientation and it initialize the structure
 * eventually used to calibrate the exposure.
 * @param camera                    The camera
 * @param exposure_calibration_size The number of frames over which to calibrate exposure and gains.
 */
void himax_configure(struct pi_device * camera, unsigned int exposure_calibration_size);

/**
 * Enable/disable the automatic exposure control
 * @param camera The camera
 * @param value  Set to 0 to disable it, any other value will enable it.
 */
void himax_enable_ae(struct pi_device *camera, uint8_t value);

/**
 * The function called at every new frame acquisition to update the exposure in case
 * of the exposure control is manual. After `exposure_calibration_size` is set the parameters
 * and thereafter is does nothing until calibration is [internally] reset.
 * @param camera The camera
 */
void himax_update_exposure(struct pi_device * camera);


#endif /* end of include guard: __CAMERA_UTILS_H__ */
