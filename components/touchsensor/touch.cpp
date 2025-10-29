#include "touch.h"
#include "driver/touch_pad.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"


volatile bool touched = false;
static uint32_t start_value;

static void calibrate_touch_pad(touch_pad_t pad)
{
    int avg = 0;
    const size_t calibration_count = 128;
    for (int i = 0; i < calibration_count; ++i) {
        uint16_t val;
        touch_pad_read(pad, &val);
        avg += val;
    }
    avg /= calibration_count;
    const int min_reading = 300;
    if (avg < min_reading) {
        printf("Touch pad #%d average reading is too low: %d (expecting at least %d). "
               "Not using for deep sleep wakeup.\n", pad, avg, min_reading);
        touch_pad_config(pad, 0);
    } else {
        int threshold = avg - 100;
        printf("Touch pad #%d average: %d, wakeup threshold set to %d.\n", pad, avg, threshold);
        touch_pad_config(pad, threshold);
    }
}

void my_touch_init(){
    ESP_ERROR_CHECK(touch_pad_init());
    touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
    touch_pad_set_voltage(TOUCH_HVOLT_2V5, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    touch_pad_config(TOUCH_PAD_GPIO4_CHANNEL, 0);
    calibrate_touch_pad(TOUCH_PAD_GPIO4_CHANNEL);
    printf("Enabling touch pad wakeup\n");
    ESP_ERROR_CHECK(esp_sleep_enable_touchpad_wakeup());
    ESP_ERROR_CHECK(esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON));
    //touch_pad_filter_start(10);
    //my_touch_tresh();
    //touch_pad_isr_register(my_touch_isr ,NULL);
}

void my_touch_tresh() {
    uint16_t touch_value;
    touch_pad_read_filtered(TOUCH_PAD_GPIO4_CHANNEL, &touch_value);
    start_value = touch_value;
    ESP_ERROR_CHECK(touch_pad_set_thresh(TOUCH_PAD_GPIO4_CHANNEL, (touch_value * (2/3))));
}

void IRAM_ATTR my_touch_isr(void *arg){
    uint32_t pad_intr = touch_pad_get_status();
    touch_pad_clear_status();
    if(pad_intr & 0x01) {
        touched = true;
    }

}

void my_touch_read(void *parameters){
    while (1) {
        uint16_t value = 0;
        touch_pad_read_filtered(TOUCH_PAD_GPIO4_CHANNEL, &value);
        if (value < start_value * 80 / 100) {
        //tft.fillScreen(ST77XX_BLACK);
        vTaskSuspendAll();
        }

        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}    


