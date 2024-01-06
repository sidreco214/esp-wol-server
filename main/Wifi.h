/*
ESP Wifi API Warping 1.1
*/

#ifndef _WIFI_H_
#define _WIFI_H_

#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"

#define WIFI_DEFAULT_MAXIMUN_RETRY 5

#ifdef __cplusplus
extern "C" {
#endif

bool wifi_is_available();

/// @brief initialize Wifi. use nvs_flash_init(), esp_event_loop_create_default() before
/// @param ssid Wifi ssid
/// @param password Wifi password
/// @param maxmun_retry The maximun number of calling esp_wifi_connect()
esp_err_t wifi_init_sta(const char* ssid, const char* password, int maxmun_retry);

#ifdef __cplusplus
}
#endif

#endif