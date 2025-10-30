#pragma once

#ifdef __cplusplus
extern "C" {
#endif    

#include "variables.h"

/**
 * @brief Initializes the touch sensor module
 *
 * Configures the GPIO pins and hardware settings required for the touch
 * sensor to operate correctly. Triggers touch wakeup.
 */
void my_touch_init();

/**
 * @brief Interrupt Service Routine for touch events
 *
 * This ISR is triggered when a touch pad generates an interrupt.
 *
 * @note CURRENTLY NOT IN USE
 */

void my_touch_isr(void *arg);

/**
 * @brief Applies the touch threshold logic
 *
 * Checks the current sensor readings against predefined thresholds and
 * updates internal flags or state accordingly.
 *
 * @note CURRENTLY NOT IN USE
 */

void my_touch_tresh();

/**
 * @brief Reads the current value from the touch sensor
 *
 * This function triggers a read from the touch sensor. It does not
 * return a value directly; instead, the scheduler will be suspended if the value is
 * a registered as a touch
 * 
 * @note CURRENTLY NOT IN USE
 */

void my_touch_read(void *parameters);

#ifdef __cplusplus
}
#endif