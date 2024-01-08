#ifndef _SPIFFS_H_
#define _SPIFFS_H_

#define SPIFFS_TAG "SPIFFS"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

esp_err_t spiffs_init(esp_vfs_spiffs_conf_t* spiffs_config);

/// @brief read from SPIFFS file system
/// @param path file path
/// @param buf buffer
/// @param buf_size buffer size
/// @return the number of bytes readed
size_t read_spiffs(const char* path, char *buf, size_t buf_size);

size_t get_spiffs_file_size(const char* path);

#ifdef __cplusplus
}
#endif

#endif