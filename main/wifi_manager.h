#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize NVS and WiFi configuration
 * 
 * If no SSID/password are stored, defaults are written to NVS.
 */
void init_wifi_config();

/**
 * @brief Load WiFi SSID and password from NVS.
 *
 * @param ssid Pointer to buffer to store SSID
 * @param ssid_len Length of SSID buffer
 * @param password Pointer to buffer to store password
 * @param pass_len Length of password buffer
 */
void load_wifi_config(char* ssid, size_t ssid_len, char* password, size_t pass_len);

/**
 * @brief Initializes WiFi SoftAP
 *
 * Sets up network interface, event loop, and starts WiFi access point.
 */
void wifi_init_softap();

#ifdef __cplusplus
}
#endif
