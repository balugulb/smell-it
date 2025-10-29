#pragma once

#include "Adafruit_ST7735.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the TFT display and start the display update task.
 */
void display_init(void);

/**
 * @brief Update the displayed message.
 *
 * This function notifies the display task to update the screen with a new message.
 *
 * @param msg Pointer to a null-terminated string to display.
 */
void start_display_task(void);

#ifdef __cplusplus
}
#endif
