/*
ESP Non Volatile Storage (NVS) C++ Warping 
Version 1.1
*/

#ifndef __ESP_NVS_HPP__
#define __ESP_NVS_HPP__

#include <string>

#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"

#define NVS_TAG "NVS"

typedef union _nvs_float_socket {
    float _float;
    int32_t _int;
} nvs_float_socket_t;

typedef union _nvs_double_socket {
    double _double;
    int64_t _int;
} nvs_double_socket_t;

static bool _nvs_is_initialized = false;

class ESP_NVS {
    private:
        nvs_handle_t nvs_handle;
        std::string nvs_namespace;
    
    public:
        ESP_NVS(std::string nvs_namespace) noexcept;
        esp_err_t init();
        esp_err_t open(nvs_open_mode_t nvs_open_mode = NVS_READWRITE);
        void close();
        esp_err_t commit();

        template<typename T>
        esp_err_t read(const char* key, T* buf);

        template<typename T>
        esp_err_t write(const char* key, T value); 
};

ESP_NVS::ESP_NVS(std::string nvs_namespace) noexcept: nvs_namespace(nvs_namespace) {};

esp_err_t ESP_NVS::init() {
    if(_nvs_is_initialized) return ESP_OK;
    esp_err_t err = nvs_flash_init();
    switch(err) {
        case ESP_OK:
            ESP_LOGI(NVS_TAG, "initialize NVS");
            _nvs_is_initialized = true;
            break;
        
        case ESP_ERR_NVS_NO_FREE_PAGES:
            ESP_LOGE(NVS_TAG, "There is no free page in NVS, please erase all");
            break;
        
        default:
            break;
    }
    return err;
};

esp_err_t ESP_NVS::open(nvs_open_mode_t nvs_open_mode) {
    esp_err_t err = nvs_open(nvs_namespace.c_str(), nvs_open_mode, &nvs_handle);
    switch(err) {
        case ESP_OK:
            ESP_LOGI(NVS_TAG, "Opening NVS, namespace: %s", nvs_namespace.c_str());
            break;
        
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGI(NVS_TAG, "%s isn't assigned", nvs_namespace.c_str());
            break;
        
        default:
            ESP_LOGE(NVS_TAG, "Error (%s)", esp_err_to_name(err));
            break;
    }
    return err;
};

void ESP_NVS::close() {
    ESP_LOGI(NVS_TAG, "Closing NVS, namespace: %s", nvs_namespace.c_str());
    nvs_close(nvs_handle);
};

esp_err_t ESP_NVS::commit() {
    esp_err_t err = nvs_commit(nvs_handle);
    if(err == ESP_OK) ESP_LOGI(NVS_TAG, "Commiting NVS, namespace: %s", nvs_namespace.c_str());
    else              ESP_LOGE(NVS_TAG, "Error (%s)", esp_err_to_name(err));
    return err;
}

template<typename T>
esp_err_t ESP_NVS::read(const char* key, T* buf) {
    ESP_LOGE("NVS", "Error Type");
    return ESP_FAIL;
};

template<>
esp_err_t ESP_NVS::read(const char *key, int8_t *buf) {
    return nvs_get_i8(nvs_handle, key, buf);
};

template<>
esp_err_t ESP_NVS::read(const char *key, uint8_t *buf) {
    return nvs_get_u8(nvs_handle, key, buf);
};

template<>
esp_err_t ESP_NVS::read(const char *key, int16_t *buf) {
    return nvs_get_i16(nvs_handle, key, buf);
};

template<>
esp_err_t ESP_NVS::read(const char *key, uint16_t *buf) {
    return nvs_get_u16(nvs_handle, key, buf);
};

template<>
esp_err_t ESP_NVS::read(const char *key, int32_t *buf) {
    return nvs_get_i32(nvs_handle, key, buf);
};

template<>
esp_err_t ESP_NVS::read(const char *key, uint32_t *buf) {
    return nvs_get_u32(nvs_handle, key, buf);
};

template<>
esp_err_t ESP_NVS::read(const char *key, int64_t *buf) {
    return nvs_get_i64(nvs_handle, key, buf);
};

template<>
esp_err_t ESP_NVS::read(const char *key, uint64_t *buf) {
    return nvs_get_u64(nvs_handle, key, buf);
};

template<>
esp_err_t ESP_NVS::read(const char *key, char *buf) {
    size_t size;
    nvs_get_str(nvs_handle, key, NULL, &size);
    return nvs_get_str(nvs_handle, key, buf,&size);
};

template<>
esp_err_t ESP_NVS::read(const char *key, float *buf) {
    nvs_float_socket_t socket = {0};
    esp_err_t err = read(key, &socket._int);
    if(socket._int) *buf = socket._float;
    return err;
};

template<>
esp_err_t ESP_NVS::read(const char *key, double *buf) {
    nvs_double_socket_t socket = {0};
    esp_err_t err = read(key, &socket._int);
    if(socket._int) *buf = socket._double;
    return err;
};

template<typename T>
esp_err_t ESP_NVS::write(const char* key, T value) {
    ESP_LOGE("NVS", "Error Type");
    return ESP_FAIL;
};

template<>
esp_err_t ESP_NVS::write(const char *key, int8_t value) {
    esp_err_t err = nvs_set_i8(nvs_handle, key, value);
    if(err == ESP_OK) ESP_LOGI(NVS_TAG, "Writing NVS, key: %s, namespace: %s", key, nvs_namespace.c_str());
    else              ESP_LOGE(NVS_TAG, "Error (%s)", esp_err_to_name(err));
    return err;
};

template<>
esp_err_t ESP_NVS::write(const char *key, uint8_t value) {
    esp_err_t err = nvs_set_u8(nvs_handle, key, value);
    if(err == ESP_OK) ESP_LOGI(NVS_TAG, "Writing NVS, key: %s, namespace: %s", key, nvs_namespace.c_str());
    else              ESP_LOGE(NVS_TAG, "Error (%s)", esp_err_to_name(err));
    return err;
};

template<>
esp_err_t ESP_NVS::write(const char *key, int16_t value) {
    esp_err_t err = nvs_set_i16(nvs_handle, key, value);
    if(err == ESP_OK) ESP_LOGI(NVS_TAG, "Writing NVS, key: %s, namespace: %s", key, nvs_namespace.c_str());
    else              ESP_LOGE(NVS_TAG, "Error (%s)", esp_err_to_name(err));
    return err;
};

template<>
esp_err_t ESP_NVS::write(const char *key, uint16_t value) {
    esp_err_t err = nvs_set_u16(nvs_handle, key, value);
    if(err == ESP_OK) ESP_LOGI(NVS_TAG, "Writing NVS, key: %s, namespace: %s", key, nvs_namespace.c_str());
    else              ESP_LOGE(NVS_TAG, "Error (%s)", esp_err_to_name(err));
    return err;
};

template<>
esp_err_t ESP_NVS::write(const char *key, int32_t value) {
    esp_err_t err = nvs_set_i32(nvs_handle, key, value);
    if(err == ESP_OK) ESP_LOGI(NVS_TAG, "Writing NVS, key: %s, namespace: %s", key, nvs_namespace.c_str());
    else              ESP_LOGE(NVS_TAG, "Error (%s)", esp_err_to_name(err));
    return err;
};

template<>
esp_err_t ESP_NVS::write(const char *key, uint32_t value) {
    esp_err_t err = nvs_set_u32(nvs_handle, key, value);
    if(err == ESP_OK) ESP_LOGI(NVS_TAG, "Writing NVS, key: %s, namespace: %s", key, nvs_namespace.c_str());
    else              ESP_LOGE(NVS_TAG, "Error (%s)", esp_err_to_name(err));
    return err;
};

template<>
esp_err_t ESP_NVS::write(const char *key, int64_t value) {
    esp_err_t err = nvs_set_i64(nvs_handle, key, value);
    if(err == ESP_OK) ESP_LOGI(NVS_TAG, "Writing NVS, key: %s, namespace: %s", key, nvs_namespace.c_str());
    else              ESP_LOGE(NVS_TAG, "Error (%s)", esp_err_to_name(err));
    return err;
};

template<>
esp_err_t ESP_NVS::write(const char *key, uint64_t value) {
    esp_err_t err = nvs_set_u64(nvs_handle, key, value);
    if(err == ESP_OK) ESP_LOGI(NVS_TAG, "Writing NVS, key: %s, namespace: %s", key, nvs_namespace.c_str());
    else              ESP_LOGE(NVS_TAG, "Error (%s)", esp_err_to_name(err));
    return err;
};

template<>
esp_err_t ESP_NVS::write(const char *key, char* str) {
    esp_err_t err = nvs_set_str(nvs_handle, key, str);
    if(err == ESP_OK) ESP_LOGI(NVS_TAG, "Writing NVS, key: %s, namespace: %s", key, nvs_namespace.c_str());
    else              ESP_LOGE(NVS_TAG, "Error (%s)", esp_err_to_name(err));
    return err;
};

template<>
esp_err_t ESP_NVS::write(const char *key, const char* str) {
    esp_err_t err = nvs_set_str(nvs_handle, key, str);
    if(err == ESP_OK) ESP_LOGI(NVS_TAG, "Writing NVS, key: %s, namespace: %s", key, nvs_namespace.c_str());
    else              ESP_LOGE(NVS_TAG, "Error (%s)", esp_err_to_name(err));
    return err;
};

template<>
esp_err_t ESP_NVS::write(const char *key, std::string str) {
    return write<const char*>(key, str.c_str());
};

template<>
esp_err_t ESP_NVS::write(const char *key, float value) {
    nvs_float_socket_t socket = {value};
    esp_err_t err = nvs_set_i32(nvs_handle, key, socket._int);
    if(err == ESP_OK) ESP_LOGI(NVS_TAG, "Writing NVS, key: %s, namespace: %s", key, nvs_namespace.c_str());
    else              ESP_LOGE(NVS_TAG, "Error (%s)", esp_err_to_name(err));
    return err;
};

template<>
esp_err_t ESP_NVS::write(const char *key, double value) {
    nvs_double_socket_t socket = {value};
    esp_err_t err = nvs_set_i64(nvs_handle, key, socket._int);
    if(err == ESP_OK) ESP_LOGI(NVS_TAG, "Writing NVS, key: %s, namespace: %s", key, nvs_namespace.c_str());
    else              ESP_LOGE(NVS_TAG, "Error (%s)", esp_err_to_name(err));
    return err;
};

#endif