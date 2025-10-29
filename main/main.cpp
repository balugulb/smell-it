/**
 * @file main.cpp
 * @brief Main program for the Smell-It project
 *
 * Initializes Wifi SoftAP, TCP-Server, TFT Display and touch module
 * 
 * Handles LCD updates, TCP communication, and deep sleep logic.
 */

/* needed components */
#include "touch.h"
#include "wifi_manager.h"
#include "tcp_server.h"
#include "display.h"
#include "variables.h"
#include "deepsleep.h"


/**
 * @brief Application entry point
 *
 * Initializes NVS, WiFi, touch sensor, TFT display, and starts tasks
 * for LCD transfer, TCP server, and deep sleep handling.
 */
extern "C" void app_main(void)
{
    init_wifi_config();
    wifi_init_softap();
    my_touch_init();
    display_init();
    init_tft_queue();

    // Start all RTOS tasks
    start_tcp_server_task();
    start_display_task();
    start_deep_sleep_task();
    //xTaskCreate(statTask, "stat task", 4096, NULL, 1, NULL);
}


/**
 * @brief Periodic task to print FreeRTOS statistics
 *
 * Prints runtime stats every 5 seconds.
 *
 * @param param Unused
 */
/*static void statTask (void *param) {
    TickType_t lastExe;
    static char strBuffer[512];
    lastExe = xTaskGetTickCount();
    for(;;) {
        xTaskDelayUntil(&lastExe, 5000);
        vTaskGetRunTimeStats(strBuffer);
        printf( "\nTask\t\tAbs\t\t\t%%\n" );
        printf("------------------------\n");
        printf("%s", strBuffer);
    }
}*/