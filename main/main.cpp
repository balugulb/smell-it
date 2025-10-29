/**
 * @file main.cpp
 * @brief Main program for the Smell-It project
 *
 * Initializes Wifi SoftAP, TCP-Server, TFT Display and touch module
 * 
 * Handles LCD updates, TCP communication, and deep sleep logic.
 */

 // my components
#include "Adafruit_ST7735.h"
#include "touch.h"

extern "C" {
    
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include <sys/param.h>
#include "esp_system.h"
#include "esp_netif.h"
#include "lwip/sockets.h"
#include <lwip/netdb.h>

#include "driver/rtc_io.h"

/* Forward declarations of internal functions */
static void do_retransmit(const int sock);
static void tcp_server_task(void *pvParameters);
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void wifi_init_softap(void);
void app_main(void); 
}

#define PORT                        3333
#define KEEPALIVE_IDLE              5
#define KEEPALIVE_INTERVAL          5
#define KEEPALIVE_COUNT             3

#define CONFIG_EXAMPLE_IPV4         1

/* ST7735 connected Pins*/
#define TFT_CS        5
#define TFT_RST        21 
#define TFT_DC         22

/** @brief Logging tag for WiFi and display messages */
static const char *TAG = "WIFI-DISPLAY";

/** @brief Message buffer for TFT display */
DMA_ATTR char msg_tft[128] = "Hello";

/** @brief RTC memory for deep sleep tracking */
static RTC_DATA_ATTR struct timeval sleep_enter_time;

/** @brief TFT display instance */
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

/** @brief Handle for LCD transfer task */
TaskHandle_t taskLcdTransfer;

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

/**
 * @brief Task to handle TFT display updates
 *
 * Clears the display, sets cursor and text attributes, and prints messages.
 * Waits for notifications to update the display content.
 *
 * @param parameter Unused
 */
static void task_lcd_transfer(void *parameter) {
    tft.fillRect(0,0,128,160, ST7735_BLACK);
    tft.setCursor(20,14);
    tft.setTextSize(3);
    tft.setTextColor(ST77XX_GREEN);  
    tft.print(msg_tft);
    uint32_t ulEventsToProcess;
    while(1) {
        ulEventsToProcess = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if( ulEventsToProcess != 0) {
            tft.fillRect(0,0,128,160, ST7735_BLACK);
            tft.setCursor(20,14);
            tft.setTextSize(3);
            tft.setTextColor(ST77XX_GREEN);  
            tft.print(msg_tft);
        }

    }
}


/**
 * @brief Filters TCP message for TFT display
 *
 * Copies input buffer until newline into output buffer for display.
 *
 * @param input Input TCP message
 * @param output Output buffer for TFT
 */
static void filter_tcp_msg(char* input, char* output) {
    memset(msg_tft, 0, sizeof(msg_tft));
    int i = 0;
    while (input[i] != '\n') {
        i++;
    }
    memcpy(output, input, i);    
 }


/**
 * @brief Handles retransmission of TCP data
 *
 * Receives data from socket, filters it for display, notifies LCD task,
 * and sends back the same data to client.
 *
 * @param sock TCP socket descriptor
 */ 
static void do_retransmit(const int sock)
{
    int len;
    char rx_buffer[128];

    do {
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        } else if (len == 0) {
            ESP_LOGW(TAG, "Connection closed");
        } else {
            filter_tcp_msg(rx_buffer, msg_tft);
            xTaskNotifyGive(taskLcdTransfer);
            rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string
            ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);

            // send() can return less bytes than supplied length.
            // Walk-around for robust implementation.
            int to_write = len;
            while (to_write > 0) {
                int written = send(sock, rx_buffer + (len - to_write), to_write, 0);
                if (written < 0) {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                    // Failed to retransmit, giving up
                    return;
                }
                to_write -= written;
            }
        }
    } while (len > 0);
}


/**
 * @brief TCP server task
 *
 * Creates a listening socket, accepts client connections, sets TCP keepalive,
 * and handles incoming messages.
 *
 * @param pvParameters Pointer to address family (AF_INET or AF_INET6)
 */
static void tcp_server_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;
    int keepAlive = 1;
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;
    struct sockaddr_storage dest_addr;

#ifdef CONFIG_EXAMPLE_IPV4
    if (addr_family == AF_INET) {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(PORT);
        ip_protocol = IPPROTO_IP;
    }
#endif
#ifdef CONFIG_EXAMPLE_IPV6
    if (addr_family == AF_INET6) {
        struct sockaddr_in6 *dest_addr_ip6 = (struct sockaddr_in6 *)&dest_addr;
        bzero(&dest_addr_ip6->sin6_addr.un, sizeof(dest_addr_ip6->sin6_addr.un));
        dest_addr_ip6->sin6_family = AF_INET6;
        dest_addr_ip6->sin6_port = htons(PORT);
        ip_protocol = IPPROTO_IPV6;
    }
#endif

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#if defined(CONFIG_EXAMPLE_IPV4) && defined(CONFIG_EXAMPLE_IPV6)
    // Note that by default IPV6 binds to both protocols, it is must be disabled
    // if both protocols used at the same time (used in CI)
    setsockopt(listen_sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
#endif

    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1) {

        ESP_LOGI(TAG, "Socket listening");

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        // Set tcp keepalive option
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
        // Convert ip address to string
#ifdef CONFIG_EXAMPLE_IPV4
        if (source_addr.ss_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }
#endif
#ifdef CONFIG_EXAMPLE_IPV6
        if (source_addr.ss_family == PF_INET6) {
            inet6_ntoa_r(((struct sockaddr_in6 *)&source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1);
        }
#endif
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        do_retransmit(sock);

        shutdown(sock, 0);
        close(sock);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}


/**
 * @brief WiFi event handler
 *
 * Handles station connect and disconnect events for the softAP.
 *
 * @param arg User argument (unused)
 * @param event_base Event base
 * @param event_id Event ID
 * @param event_data Event-specific data
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
}


/**
 * @brief Initializes WiFi SoftAP
 *
 * Sets up network interface, event loop, and starts WiFi access point.
 */
void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "WIFI-ESP",
            .password = "87654321",
            .ssid_len = strlen("WIFI-ESP"),
            .channel = 1,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
            .authmode = WIFI_AUTH_WPA3_PSK,
            //.sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else /* CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT */
            .authmode = WIFI_AUTH_WPA2_PSK,
#endif
            .max_connection = 4,

            .pmf_cfg = {
                    .required = true,
            },
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#endif
        },
    };
    if (strlen("87654321") == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             "WIFI-ESP", "87654321", 1);
}


/**
 * @brief Periodic task to print FreeRTOS statistics
 *
 * Prints runtime stats every 5 seconds.
 *
 * @param param Unused
 */
static void statTask (void *param) {
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
}


/**
 * @brief Application entry point
 *
 * Initializes NVS, WiFi, touch sensor, TFT display, and starts tasks
 * for LCD transfer, TCP server, and deep sleep handling.
 */
void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();
    my_touch_init();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    tft.initR(INITR_BLACKTAB);  
    xTaskCreate(&task_lcd_transfer, "Blink LCD", 4096, NULL, 1, &taskLcdTransfer); 
    xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET, 5, NULL);
    xTaskCreate(deep_sleep_task,"deep sleep task", 4096, NULL, 6, NULL);
    //xTaskCreate(statTask, "stat task", 4096, NULL, 1, NULL);
}
