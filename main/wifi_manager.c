#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include <string.h>

/* Default WiFi credentials */
#define DEFAULT_SSID "WIFI_ESP"
#define DEFAULT_PASSWORD "87654321"

/** @brief Logging tag for wifi_manager */
static const char *TAG = "wifi";

/** -----------------------------------------------------------------------------------------------------------------------------------------------------------------*/

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


void init_wifi_config() {
    // NVS init
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Open NVS
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open("wifi_config", NVS_READWRITE, &my_handle));

    // Check for existing SSID
    size_t ssid_len = 0;
    ret = nvs_get_str(my_handle, "ssid", NULL, &ssid_len);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        // Write WIFI default values
        ESP_ERROR_CHECK(nvs_set_str(my_handle, "ssid", DEFAULT_SSID));
        ESP_ERROR_CHECK(nvs_set_str(my_handle, "password", DEFAULT_PASSWORD));
        ESP_ERROR_CHECK(nvs_commit(my_handle));
        ESP_LOGI("WiFi", "Default WiFi credentials written to NVS");
    }

    nvs_close(my_handle);
}


void load_wifi_config(char* ssid, size_t ssid_len, char* password, size_t pass_len) {
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open("wifi_config", NVS_READONLY, &my_handle));

    ESP_ERROR_CHECK(nvs_get_str(my_handle, "ssid", ssid, &ssid_len));
    ESP_ERROR_CHECK(nvs_get_str(my_handle, "password", password, &pass_len));

    nvs_close(my_handle);
}


void wifi_init_softap(void)
{
    ESP_LOGI(TAG, "Initializing WiFi SoftAP...");

    // Basic system init 
    ESP_ERROR_CHECK(esp_netif_init());

    // Event loop check
    static bool event_loop_created = false;
    if (!event_loop_created) {
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        event_loop_created = true;
    }

    // Create WIFI netif
    esp_netif_create_default_wifi_ap();

    // WIFI init config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init() failed: %s", esp_err_to_name(ret));
        return;
    }

    // Event handler
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    // Load WIFI config
    char ssid[32];
    char password[64];
    load_wifi_config(ssid, sizeof(ssid), password, sizeof(password));

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));
    strncpy((char *)wifi_config.ap.password, password, sizeof(wifi_config.ap.password));
    wifi_config.ap.ssid_len = (uint8_t)strlen(ssid);
    wifi_config.ap.channel = 1;
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.authmode = strlen(password) >= 8 ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
    wifi_config.ap.pmf_cfg.required = true;

    // Set mode to AP
    ret = esp_wifi_set_mode(WIFI_MODE_AP);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_mode failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_config failed: %s", esp_err_to_name(ret));
        return;
    }
    // Start WIFI
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_start failed: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "SoftAP started successfully. SSID: %s  PASSWORD: %s", ssid, password);
}