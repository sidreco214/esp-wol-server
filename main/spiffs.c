#include "spiffs.h"

esp_err_t spiffs_init(esp_vfs_spiffs_conf_t* spiffs_config) {
    ESP_LOGI(SPIFFS_TAG, "Initializing SPIFFS");

    esp_err_t err = esp_vfs_spiffs_register(spiffs_config);
    switch(err) {
        default:
            ESP_LOGE(SPIFFS_TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(err));
            return err;
        
        case ESP_FAIL:
            ESP_LOGE(SPIFFS_TAG, "Failed to mount or format file-system");
            return err;
        
        case ESP_ERR_NOT_FOUND:
            ESP_LOGE(SPIFFS_TAG, "Failed to find SPIFFS partitions");
            return err;
        
        case ESP_OK:
            break;
    }

    ESP_LOGI(SPIFFS_TAG, "SPIFFS Check");
    err = esp_spiffs_check(spiffs_config->partition_label);
    if(err != ESP_OK) {
        ESP_LOGE(SPIFFS_TAG, "Failed to check (%s)", esp_err_to_name(err));
        return err;
    }

    size_t total = 0, used = 0;
    err = esp_spiffs_info(spiffs_config->partition_label, &total, &used);
    if(err == ESP_OK) {
        ESP_LOGI(SPIFFS_TAG, "Partition: toal %d, used %d", total, used);
        return ESP_OK;
    } else {
        ESP_LOGE(SPIFFS_TAG, "Failed to get SPIFFS partition information (%s). Please format", esp_err_to_name(err));
        return err;
    }
}

size_t get_spiffs_file_size(const char* path) {
    FILE* fp = fopen(path, "r");
    if(fp == NULL) {
        ESP_LOGE(SPIFFS_TAG, "Failed to open %s", path);
        return 0;
    }
    fseek(fp, 0, SEEK_END);
    size_t fsize = ftell(fp);
    fclose(fp);
    return fsize;
}

size_t read_spiffs(const char* path, char *buf, size_t buf_size) {
    FILE* fp = fopen(path, "r");
    if(fp == NULL) {
        ESP_LOGE(SPIFFS_TAG, "Failed to open %s", path);
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    size_t fsize = ftell(fp);
    if(buf_size < fsize) {
        ESP_LOGE(SPIFFS_TAG, "Too small buf");
        return 0;
    }

    fseek(fp, 0, SEEK_SET);
    for(int i=0; i < fsize; ++i) {
        buf[i] = fgetc(fp);
    }
    buf[fsize-1] = '\0';
    fclose(fp);
    return fsize;
}