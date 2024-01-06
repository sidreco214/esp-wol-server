#include "util.h"

esp_err_t mac_addr_parsing(std::string mac_str, mac_addr_t& mac) {
    if(mac_str.size() != 17) return ESP_FAIL;
    /*
        char to hex
        1.lowercase -> uppercase (toupper)
                        
        2.ascii code
        '0' = 48
        '9' = 57
        'A' = 65
        'F' = 70
        char - '0'
        if char > 16, char -= 7 ('A' - '0' + 10 )
    */
    char arr[18];
    for(int i=0; i<6; ++i) {
        arr[3*i] = toupper(mac_str[3*i]);
        uint8_t high = arr[3*i] - '0';
        if(high > 16) high -= 7;
        if(high <= 0 || high > 16) return ESP_FAIL;

        arr[3*i+1] = toupper(mac_str[3*i+1]);
        uint8_t low = arr[3*i+1] - '0';
        if(low > 16) low -= 7;
        if(low <= 0 || low > 16) return ESP_FAIL;

        mac.addr[i] = (high << 4) + low;
    }
    return ESP_OK;
};
