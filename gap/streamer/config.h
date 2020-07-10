#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "camera.h"

#define TARGET_INTENSITY 112 // = 0x70

#ifdef GVSOC
// Targeting GVSOC ...
// The image used to feed the camera is 324 x 244
#define MARGIN_TOP 74
#define MARGIN_BOTTOM 74
#define MARGIN_LEFT 82
#define MARGIN_RIGHT 82
#define DSMPL_RATIO 1  // Down-sampling ratio (after cropping)
#define FORMAT QVGA

#else

#define MARGIN_TOP 33
#define MARGIN_BOTTOM 33
#define MARGIN_LEFT 0
#define MARGIN_RIGHT 2
#define DSMPL_RATIO 1  // Down-sampling ratio (after cropping)
#define FORMAT HALF

#endif
