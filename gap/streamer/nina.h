/**
 * This header defines the interfaces to initialize and close the NINA chip
 *
 * Author: Jérôme Guzzi (jerome@idsia.ch)
 * License: MIT
 */

#ifndef __NINA_H__
#define __NINA_H__

#include "pmsis.h"

/**
 * Initialize the GPIO pin used to wake up NINA
 * @param value The initial value of the GPIO pin. Set to 1 to wake up NINA immediately.
 */
void init_nina(int active);

/**
 * Open the WiFi device: wake up NINA and than set up the [SPI] connection
 * @return      The device or NULL in case of failure
 */
struct pi_device * open_wifi();

/**
 * Close the interface. At the moment does not put NINA to sleep.
 */
void close_nina();


// void wake_up_nina();
// void put_nina_to_sleep();


#endif // __NINA_H__
