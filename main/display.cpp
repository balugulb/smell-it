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
    char msg[TFT_MSG_SIZE];
   
    // Init screen (all black)
    tft.fillRect(0,0,128,160, ST7735_BLACK);

    while(1) {
        if(xQueueReceive(tftQueue, &msg, portMAX_DELAY)) {
            tft.fillRect(0, 0, 128, 160, ST7735_BLACK);
            tft.setCursor(TEXT_X, TEXT_Y);
            tft.setTextSize(3);
            tft.setTextColor(ST77XX_GREEN);  
            tft.print(msg);
        }
    }
}


void start_display_task(){
    xTaskCreate(&task_lcd_transfer, "TFT", 4096, NULL, 1, &taskLcdTransfer); 
}