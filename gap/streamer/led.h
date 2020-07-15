/**
 * This header provides a simple interfaces to control the LED connected via GPIO 2 to the AI DECK.
 *
 * Author: Jérôme Guzzi (jerome@idsia.ch)
 * License: MIT
 */

#ifndef __LED_H__
#define __LED_H__

/**
 * Initialize the LED GPIO
 */
void init_led();

/**
 * Set the LED state
 * @param value The LED value: 0 to switch it off, any other value to switch it on.
 */
void set_led(int value);

/**
 * Toggle the LED state
 */
void toggle_led();

#endif // __LED_H__
