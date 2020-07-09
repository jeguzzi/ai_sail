/*----------------------------------------------------------------------------*
 * Copyright (C) 2018-2019 ETH Zurich, Switzerland                            *
 * All rights reserved.                                                       *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License");            *
 * you may not use this file except in compliance with the License.           *
 * See LICENSE.apache.md in the top directory for details.                    *
 * You may obtain a copy of the License at                                    *
 *                                                                            *
 *     http://www.apache.org/licenses/LICENSE-2.0                             *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 *                                                                            *
 * File:    config.h                                                          *
 * Author:  Daniele Palossi <dpalossi@iis.ee.ethz.ch>                         *
 * Date:    10.04.2019                                                        *
 *----------------------------------------------------------------------------*/

#ifndef PULP_FRONTNET_CONFIG
#define PULP_FRONTNET_CONFIG

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
