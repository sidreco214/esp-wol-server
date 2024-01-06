#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdint.h>

#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"

typedef union {
    uint8_t addr[6];
    int64_t data;
} mac_addr_t;

esp_err_t mac_addr_parsing(std::string mac_str, mac_addr_t& mac);

#endif