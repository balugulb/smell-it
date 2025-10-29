#include "variables.h"
#include "esp_log.h"


/** @brief Logging tag for variables*/
static const char *TAG = "var";


QueueHandle_t tftQueue = nullptr;

void init_tft_queue() {
    tftQueue = xQueueCreate(TFT_QUEUE_LENGTH, TFT_MSG_SIZE);
    if (tftQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create TFT message queue");
    }
}
