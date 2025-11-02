#include "display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>
#include "variables.h"


/** @brief Logging tag for display */
static const char *TAG = "display";

/** @brief Handle for LCD transfer task */
TaskHandle_t taskLcdTransfer;

/** @brief TFT display instance */
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

/**
 * @brief Init TFT screen (all black) 
 */
void display_init(void){
    tft.initR(INITR_BLACKTAB);
}


/**
 * @brief Task to handle TFT display updates
 *
 * Clears the display, sets cursor and text attributes, and prints messages.
 * Waits for notifications to update the display content.
 *
 * @param parameter Unused
 */
static void task_lcd_transfer(void *parameter) {
    display_msg_t display_msg;
   
    // Init screen (all black)
    tft.fillRect(0,0,128,160, ST7735_BLACK);

    while(1) {
        if(xQueueReceive(displayQueue, &display_msg, portMAX_DELAY)) {

            if(display_msg.type == MSG_TCP) {
                tft.fillRect(TEXT_TCP_X, TEXT_TCP_Y, TEXT_TCP_WIDTH, TEXT_TCP_HEIGHT, ST7735_BLACK);
                tft.setCursor(TEXT_TCP_X + 5, TEXT_TCP_Y + 5);
                tft.setTextSize(3);
                tft.setTextColor(ST77XX_GREEN);  
                tft.print(display_msg.data.tcp_msg);
            }

            if(display_msg.type == MSG_MQ2) {
                tft.fillRect(TEXT_MQ2_X, TEXT_MQ2_Y, TEXT_MQ2_WIDTH, TEXT_MQ2_HEIGHT, ST7735_BLACK);
                tft.setCursor(TEXT_MQ2_X, TEXT_MQ2_Y);
                tft.setTextSize(2);
                tft.setTextColor(ST77XX_RED);
                tft.printf("LPG:%.1f\nCO:%.1f\nSMK:%.1f", display_msg.data.mq2.lpg, display_msg.data.mq2.co, display_msg.data.mq2.smoke);
            }
        }
    }
}


void start_display_task(){
    xTaskCreate(&task_lcd_transfer, "TFT", 4096, NULL, 1, &taskLcdTransfer); 
}