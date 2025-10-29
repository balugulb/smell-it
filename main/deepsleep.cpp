#include "deepsleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sys/time.h"
#include "esp_system.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include <stdio.h>


/** @brief RTC memory for deep sleep tracking */
static RTC_DATA_ATTR struct timeval sleep_enter_time;


/**
 * @brief Task to handle deep sleep
 *
 * Calculates sleep duration, prints debug info, isolates GPIOs, and
 * enters deep sleep mode.
 *
 * @param args Unused
 */
static void deep_sleep_task(void *args)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    int sleep_time_ms = (now.tv_sec - sleep_enter_time.tv_sec) * 1000 + (now.tv_usec - sleep_enter_time.tv_usec) / 1000;

    switch (esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_TOUCHPAD: {
            printf("Wake up from touch on pad %d\n", esp_sleep_get_touchpad_wakeup_status());
            printf("Sleep time: %dms\n", sleep_time_ms);
            break;
        }

        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            printf("Not a deep sleep reset\n");
    }

    vTaskDelay(300000 / portTICK_PERIOD_MS);

    rtc_gpio_isolate(GPIO_NUM_12);

    printf("Entering deep sleep\n");

    // get deep sleep enter time
    gettimeofday(&sleep_enter_time, NULL);

    // enter deep sleep
    esp_deep_sleep_start();
}


void start_deep_sleep_task(){
    xTaskCreate(deep_sleep_task,"deep sleep task", 4096, NULL, 6, NULL);
}