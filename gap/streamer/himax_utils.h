#ifndef __CAMERA_UTILS_H__
#define __CAMERA_UTILS_H__

#include "pmsis.h"

#define QVGA 0   // 324 x 244
#define QQVGA 1  // 162 x 122
#define FULL 2   // 324 x 324
#define HALF 3   // 162 x 162

void himax_set_format(struct pi_device * camera, uint8_t format);
void himax_set_target_value(struct pi_device * camera, uint8_t value);
void himax_set_fps(struct pi_device * camera, uint8_t fps, uint8_t format);
void himax_configure(struct pi_device * camera, unsigned int exposure_calibration_size);
void himax_update_exposure(struct pi_device * camera);
int himax_open(struct pi_device *camera);
void himax_close(struct pi_device *camera);
void himax_enable_ae(struct pi_device *camera, uint8_t value);

#endif /* end of include guard: __CAMERA_UTILS_H__ */


// TODO (Jerome): exposure: set larger max integration and a/d gains
