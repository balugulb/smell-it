#include "gas_sensor.h"
#include "variables.h"
#include "display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


/** @brief MQ2 sensor instance */
static MQ2 mq2_sensor = MQ2(MQ2_ADC_CHANNEL);


/**
 * @brief Task to handle MQ2 gas sensor reads
 *
 * Clears the display, sets cursor and text attributes, and prints messages.
 *
 * @param parameter Unused
 */
static void mq2_task(void* param) {

    display_msg_t msg;
    msg.type = MSG_MQ2;
    for (;;) {

        mq2_sensor.handleRecalibration();
        float* value = mq2_sensor.read(false);    
        msg.data.mq2.lpg   = value[0];
        msg.data.mq2.co    = value[1];
        msg.data.mq2.smoke = value[2];

        //printf("LPG:%.1f\nCO:%.1f\nSMK:%.1f\n", value[0], value[1], value[2]);

        // display update
        if (xQueueSend(displayQueue, &msg, portMAX_DELAY) != pdPASS) {
            ESP_LOGW("MQ2", "Display queue full, message dropped");
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}


void start_gas_sensor_task() {
    mq2_sensor.begin();
    xTaskCreate(mq2_task, "mq2_task", 4096, NULL, 2, NULL);
}
