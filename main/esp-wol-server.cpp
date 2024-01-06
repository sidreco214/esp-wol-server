//#define DEBUG
#include <stdio.h>
#include <sys/param.h>
#include <malloc.h>

#include <string>
#include <sstream>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <map>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_err.h"
#include "esp_log.h"

#include "nvs_flash.h"
#include "nvs_w.hpp"

#include "esp_https_server.h"
#include "esp_tls.h"
#include "esp_tls_crypto.h"

#include "driver/gpio.h"
#define LED_BUILTIN GPIO_NUM_2

#include "util.h"

#include "UsbSerial.h"
#define SERIAL_BAUD 115200

#include "Wifi.h"

#include "udp_client.h"
#define ESP_HTTPS_SERVER_TAG "HTTPS Server"

#define WOL_PORT 9
#define IP_ADDR_FAMILY AF_INET //ipv4: AF_INET     ipv6: AF_INET6
#define IP_PROTOCOL IPPROTO_IP //ipv4: IPPROTO_IP  ipv6: IPPROTO_IPV6
#define UDP_SEND_TIMEOUT 10

#define HTTPD_POST_BUF_SIZE 100

#define NVS_STROAGENAME "Storage"

/*
struct of nvs
namespace NVS_STROAGENAME
key            value
WIFI_SSID     string
WIFI_PW       string
BROADCAST_IP  string

namespace PC_NAME(User Register)
key            value
pw            string
mac           int32_t
*/

/*
wifi info buf
*/
char wifi_ssid[30]; //wifi ssid
char wifi_pw[30]; //wifi pw
char broadcast_ip[20]; //broadcast ip

/*
Mutexs
*/
std::shared_mutex nvs_mutex; //nvs mutex
std::mutex builtin_led_mutex;

void blink_builtin_led_task(void* pvParamter) {
    builtin_led_mutex.lock();
    for(int i=0; i<3; ++i) {
        gpio_set_level(LED_BUILTIN, 0);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        gpio_set_level(LED_BUILTIN, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    builtin_led_mutex.unlock();
    vTaskDelete(NULL);
}

static httpd_handle_t start_web_server();
inline esp_err_t stop_webserver(httpd_handle_t* httpd_handle) {
    return httpd_ssl_stop(*httpd_handle);
}

/*
Serial command
*/
//NVS Keys
typedef enum {
    NVS_KEY_WIFI_SSID,
    NVS_KEY_WIFI_PW,
    NVS_KEY_BROADCAST_IP,
    NVS_KEY_UNKNOWN
} nvs_key_t;

typedef struct {
    std::string command;
    std::string format;
    std::string explain;
} Serial_command_info_t;

typedef enum {
    SERIAL_COMMAND_HELP,
    SERIAL_COMMAND_REBOOT,
    SERIAL_COMMAND_ERASE,
    SERIAL_COMMAND_NVS_SET,
    SERIAL_COMMAND_NVS_REGISTER_USER,
    SERIAL_COMMAND_NVS_SHOW_USER,
    SERIAL_COMMNAND_UNKNOWN
} Serial_Commands_t;

int serial_help_max_len = 0;

const std::map<std::string, int> nvs_keys_map = {
    {"WIFI_SSID", NVS_KEY_WIFI_SSID},
    {"WIFI_PW", NVS_KEY_WIFI_PW},
    {"BROADCAST_IP", NVS_KEY_BROADCAST_IP},
};

Serial_command_info_t Serial_command_info[] = {
    {"/help", " ", "Show a list of serial command"},
    {"/reboot", " ", "Rebooting device"},
    {"/nvs_erase", " ", "Erase all NVS data"},
    {"/nvs_set", "<key> <value>", "Set NVS data"},
    {"/nvs_register_user", "<UserName> <password> <MAC Address>", "Register user data"},
    {"/nvs_show_user", "<UserName>", "show nvs data"}
};

const std::map<std::string, int> serial_map = {
    {"/help", SERIAL_COMMAND_HELP},
    {"/reboot", SERIAL_COMMAND_REBOOT},
    {"/nvs_erase", SERIAL_COMMAND_ERASE},
    {"/nvs_set", SERIAL_COMMAND_NVS_SET},
    {"/nvs_register_user", SERIAL_COMMAND_NVS_REGISTER_USER},
    {"/nvs_show_user", SERIAL_COMMAND_NVS_SHOW_USER}
};

void serial_command_task(ESP_NVS& nvs, std::shared_mutex& nvs_mutex);

//Basic auth
typedef enum {
    HTTPD_BASIC_AUTH_FAIL,
    HTTPD_BASIC_AUTH_NO_HEADER,
    HTTPD_BASIC_AUTH_NO_VALUE,
    HTTPD_BASIC_AUTH_NOT_MATCHED,
    HTTPD_BASIC_AUTH_SUCCESS
} auth_info_t;
static esp_err_t http_auth_basic_encode(const char* username, const char* password, char* dest, size_t dest_size);
static auth_info_t check_authencation(httpd_req_t* req, const char* id, const char* pw);

extern "C" void app_main(void) {
    gpio_set_direction(LED_BUILTIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_BUILTIN, 0);

    uart0_init(SERIAL_BAUD);

    ESP_NVS nvs(NVS_STROAGENAME);
    nvs_mutex.lock_shared();
    ESP_ERROR_CHECK(nvs.init());
    ESP_ERROR_CHECK(nvs.open());

    //if nvs key doesn't assigned, these values are not modified
    snprintf(wifi_ssid, 30, "undefined");
    snprintf(wifi_pw, 30, "undefined");
    snprintf(broadcast_ip, 20, "000.000.000.000");
    nvs.read("WIFI_SSID", wifi_ssid);
    nvs.read("WIFI_PW", wifi_pw);
    nvs.read("BROADCAST_IP", broadcast_ip);
    nvs.close();
    nvs_mutex.unlock();
    printf("NVS Wifi SSID:%s, Wifi PW:%s, Broadcast ip:%s\n", wifi_ssid, wifi_pw, broadcast_ip);
    
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_sta(wifi_ssid, wifi_pw, WIFI_DEFAULT_MAXIMUN_RETRY);

    //caculate max length of Serial_command_info.explain 
    for(auto iter : Serial_command_info) {
        int temp = iter.command.length() + iter.format.length();
        serial_help_max_len = (temp > serial_help_max_len)? temp : serial_help_max_len;
    }

    httpd_handle_t server = NULL; //wol https server handle
    while(true) {
        if(wifi_is_available()) {
            gpio_set_level(LED_BUILTIN, 1);
            if(server == NULL) server = start_web_server();
        }
        else {
            gpio_set_level(LED_BUILTIN, 0);
            if(server) stop_webserver(&server);
        }
        serial_command_task(std::ref(nvs), std::ref(nvs_mutex));
    }
}


//Basic auth
static esp_err_t http_auth_basic_encode(const char* username, const char* password, char* dest, size_t dest_size) {
    //credential = <username>:<password>
    //return Basic <encoded message>
    
    size_t out = 0;
    size_t credential_len = strlen(username) + strlen(password) + 1; // :
    char* credential = (char*)calloc(sizeof(char), credential_len + 1); // \0
    if(!credential) return ESP_ERR_NO_MEM;

    size_t size = credential_len + 7; //7 = strlen("Basic ") + \0
    if(dest_size < size) {
        free(credential);
        return ESP_FAIL;
    }

    sprintf(credential, "%s:%s", username, password);
    strcpy(dest, "Basic ");
    esp_crypto_base64_encode((unsigned char*)dest + 6, dest_size, &out, (const unsigned char*)credential, credential_len);
    free(credential);

    return ESP_OK;
}

static auth_info_t check_authencation(httpd_req_t* req, const char* id, const char* pw) {
    char* buf = NULL;
    size_t buf_len = 0;

    buf_len = httpd_req_get_hdr_value_len(req, "Authorization") + 1;
    if(buf_len <= 1) return HTTPD_BASIC_AUTH_NO_HEADER;
    
    buf = (char*)malloc(buf_len);
    if(!buf) {
        ESP_LOGE(ESP_HTTPS_SERVER_TAG, "Not enough memory");
        return HTTPD_BASIC_AUTH_FAIL;
    }

    if (httpd_req_get_hdr_value_str(req, "Authorization", buf, buf_len) != ESP_OK) { 
        free(buf);
        return HTTPD_BASIC_AUTH_NO_VALUE;
    }

    ESP_LOGI(ESP_HTTPS_SERVER_TAG, "Found Authorization header: %s", buf);

    char auth_buf[100];
    esp_err_t err = http_auth_basic_encode(id, pw, auth_buf, 100);
    if(err == ESP_ERR_NO_MEM) {
        ESP_LOGE(ESP_HTTPS_SERVER_TAG, "Not enough memory");
        free(buf);
        return HTTPD_BASIC_AUTH_FAIL;
    } else if(err == ESP_FAIL) {
        ESP_LOGE(ESP_HTTPS_SERVER_TAG, "Buffer overflow");
        return HTTPD_BASIC_AUTH_FAIL;
    }

    #ifdef DEBUG
    printf("Server: %s, received: %s\n", auth_buf, buf);
    #endif

    if(strncmp(buf, auth_buf, buf_len) == 0) {
        free(buf);
        return HTTPD_BASIC_AUTH_SUCCESS;
    }else {
        free(buf);
        return HTTPD_BASIC_AUTH_NOT_MATCHED;
    }
}

//Server URI handler
//root uri
static esp_err_t root_get_uri_handler(httpd_req_t* req) {
    ESP_LOGI(ESP_HTTPS_SERVER_TAG, "HTTPS GET /");
    httpd_resp_set_type(req, "text/html");
    extern const char root_get_html_start[] asm("_binary_root_get_html_start");
    extern const char root_get_html_end[] asm("_binary_root_get_html_end");
    size_t root_get_html_len = root_get_html_end - root_get_html_start;
    printf("Res: %s\n", root_get_html_start);
    httpd_resp_send(req, (const char*)root_get_html_start, root_get_html_len);
    return ESP_OK;
};

static esp_err_t wol_post_uri_handler(httpd_req_t* req) {
    ESP_LOGI(ESP_HTTPS_SERVER_TAG, "HTTPS POST /wol");

    char buf[HTTPD_POST_BUF_SIZE];
    int recv = 0;
    int remaining = req->content_len;

    //read data
    while(remaining > 0) {
        if((recv = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)-1))) <= 0) {
            if(recv == HTTPD_SOCK_ERR_TIMEOUT) continue; //retry receving if timeout
            return ESP_FAIL; //ret == 0 refer to closed connection and negative return mean error
        }
        remaining -= recv;
        buf[recv] = '\0';
        ESP_LOGI(ESP_HTTPS_SERVER_TAG, "Received %.*s", recv, buf);
    }

    /*
    wol post form
    Header
    Content-Type: application/x-www-form-urlencoded
    Authorization: Basic <base64 encoded credential>

    Body
    name=PC_NAME

    1.parsing
    2.authenication
    3.create task sending magic packet
    */
    char name[30];
    strlcpy(name, (char*)(buf + 5), 30);
    ESP_LOGI(ESP_HTTPS_SERVER_TAG, "WOL Request: name=%s", name);
    mac_addr_t mac;

    {
        std::shared_lock<std::shared_mutex> lk(nvs_mutex);
        ESP_NVS user_nvs(name);
        esp_err_t err = user_nvs.open(NVS_READONLY);
        if(err == ESP_ERR_NVS_NOT_FOUND) {
            //Unregistered User
            lk.unlock();
            httpd_resp_send_404(req);
            ESP_LOGI(ESP_HTTPS_SERVER_TAG, "404 Unregistered user");
            return ESP_OK;
        } else if(err != ESP_OK) {
            lk.unlock();
            httpd_resp_send_500(req);
            ESP_LOGE(ESP_HTTPS_SERVER_TAG, "500 Server Error (%s)", esp_err_to_name(err));
        }

        //Authentication
        char pw[30];
        user_nvs.read("pw", pw);
        auth_info_t auth_info = check_authencation(req, name, pw);
        switch(auth_info) {
            case HTTPD_BASIC_AUTH_FAIL:
                ESP_LOGE(ESP_HTTPS_SERVER_TAG, "Fail to basic auth");
                httpd_resp_send_500(req);
                return ESP_FAIL;
                break;
        
            case HTTPD_BASIC_AUTH_NO_HEADER:
                ESP_LOGE(ESP_HTTPS_SERVER_TAG, "Can't find Authorization Header");
                httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=wol");
                httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Can't find Authorization Header");
                return ESP_OK;
                break;
        
            case HTTPD_BASIC_AUTH_NO_VALUE:
                ESP_LOGE(ESP_HTTPS_SERVER_TAG, "Can't find Authorization Value");
                httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=wol");
                httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Can't find Authorization Value");
                return ESP_OK;
                break;
        
            case HTTPD_BASIC_AUTH_NOT_MATCHED:
                ESP_LOGI(ESP_HTTPS_SERVER_TAG, "Authentication informations aren't matched");
                httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=wol");
                httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Authentication informations aren't matched");
                return ESP_OK;
                break;
        
            case HTTPD_BASIC_AUTH_SUCCESS:
                ESP_LOGI(ESP_HTTPS_SERVER_TAG, "Authentication success");
                user_nvs.read("mac", &mac.data);
                break;
        }
    }
    
    ESP_LOGI(UDP_CLIENT_TAG, "Send Magic Packet on Broadcast:%s , MAC:%x-%x-%x-%x-%x-%x",
        broadcast_ip,
        mac.addr[0],
        mac.addr[1],
        mac.addr[2],
        mac.addr[3],
        mac.addr[4],
        mac.addr[5]
        );

    //create magic packet
    char magic_packet[103] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    for(char* packet = magic_packet+6; packet < magic_packet + 102; packet += 6)
        memcpy(packet, mac.addr, sizeof(mac.addr));
    
    //send magic packet
    udp_client_send_task_param_t udp_task_param;
    udp_task_param.dest_ip_addr = broadcast_ip;
    udp_task_param.dest_port = WOL_PORT;
    udp_task_param.addr_family = IP_ADDR_FAMILY;
    udp_task_param.ip_protocol = IP_PROTOCOL;
    udp_task_param.timeout_sec = UDP_SEND_TIMEOUT;
    udp_task_param.message = magic_packet;
    xTaskCreate(udp_client_send_task, "Send Magic Packet", 4096, &udp_task_param, 5, NULL);
    xTaskCreate(blink_builtin_led_task, "blink built-in led", 1024, NULL, 5, NULL);

    //end response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
};

static httpd_handle_t start_web_server() {
    httpd_handle_t httpd_handle;
    ESP_LOGI(ESP_HTTPS_SERVER_TAG, "Starting Server");

    //ssl config
    httpd_ssl_config_t ssl_config = HTTPD_SSL_CONFIG_DEFAULT();
    extern const unsigned char servercert_start[] asm("_binary_servercert_pem_start");
    extern const unsigned char servercert_end[] asm("_binary_servercert_pem_end");
    ssl_config.servercert = servercert_start;
    ssl_config.servercert_len = servercert_end - servercert_start;

    extern const unsigned char prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
    extern const unsigned char prvtkey_pem_end[] asm("_binary_prvtkey_pem_end");
    ssl_config.prvtkey_pem = prvtkey_pem_start;
    ssl_config.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

    esp_err_t err = httpd_ssl_start(&httpd_handle, &ssl_config);
    if(err != ESP_OK) {
        ESP_LOGE(ESP_HTTPS_SERVER_TAG, "Error starting server");
        return NULL;
    }

    ESP_LOGI(ESP_HTTPS_SERVER_TAG, "Registering URI handler");
    httpd_uri_t root_uri;
    root_uri.uri = "/";
    root_uri.method = HTTP_GET;
    root_uri.handler = root_get_uri_handler;
    ESP_ERROR_CHECK(httpd_register_uri_handler(httpd_handle, &root_uri));

    httpd_uri_t wol_post_uri;
    wol_post_uri.uri = "/wol";
    wol_post_uri.method = HTTP_POST;
    wol_post_uri.handler = wol_post_uri_handler;
    ESP_ERROR_CHECK(httpd_register_uri_handler(httpd_handle, &wol_post_uri));
    ESP_LOGI(ESP_HTTPS_SERVER_TAG, "Server configuration done");
    return httpd_handle;
};

void serial_command_task(ESP_NVS& nvs, std::shared_mutex& nvs_mutex) {
    char serial_buf[128];
    if(!uart0_readData(serial_buf)) return;

    //parsing
    std::vector<std::string> args;
    args.reserve(5);
    {
        std::stringstream ss(serial_buf);
        for(std::string token; getline(ss, token, ' ');) args.push_back(token);
    }

    #ifdef DEBUG
    for(auto iter : args) {
        printf("receving:%s\n", iter.c_str());
    }
    #endif

    int command = SERIAL_COMMNAND_UNKNOWN;
    auto find = serial_map.find(args[0]);
    if(find != serial_map.end()) command = find->second;

    switch(command) {
        default:
            break;
        
        case SERIAL_COMMNAND_UNKNOWN:
            printf("Unknown Command\n");
            break;
        
        case SERIAL_COMMAND_HELP:
            for(auto iter : Serial_command_info) {
                printf("%-*s %s\n",
                    serial_help_max_len+5,
                    (iter.command+ " " + iter.format).c_str(),
                    iter.explain.c_str()
                );
            }
            break;
        
            case SERIAL_COMMAND_REBOOT:
                fflush(stdout);
                printf("Rebooting ...\n");
                vTaskDelay(3000 / portTICK_PERIOD_MS);
                esp_restart();
                break;
            
            case SERIAL_COMMAND_ERASE:
                {
                    std::scoped_lock<std::shared_mutex> lk(nvs_mutex);
                    ESP_LOGI("NVS", "Erase all nvs_data\n");
                    ESP_ERROR_CHECK(nvs_flash_erase());
                    ESP_ERROR_CHECK(nvs_flash_init());
                    fflush(stdout);
                    printf("Rebooting ...\n");
                    vTaskDelay(3000 / portTICK_PERIOD_MS);
                    esp_restart();
                }
                break;
            
            case SERIAL_COMMAND_NVS_SET:
                {
                    //      /nvs_set <key> <value>
                    // args     0      1      2
                    auto nvs_key = nvs_keys_map.find(args[1]);
                    if(nvs_key == nvs_keys_map.end()) {
                        printf("Unknown NVS key\n==============\nKey list\n");
                        for(auto iter : nvs_keys_map) printf("%s\n", iter.first.c_str());    
                    }
                    else if(args.size() < 3) {
                        printf("Error: Input value\n");
                    }
                    else {
                        std::scoped_lock<std::shared_mutex> lk(nvs_mutex);
                        nvs.open();
                        nvs.write(args[1].c_str(), args[2].c_str());
                        nvs.commit();
                        nvs.close();
                        printf("Please /reboot\n");
                    }
                }
                break;
            
            case SERIAL_COMMAND_NVS_REGISTER_USER:
                //      /nvs_register_user <UserName> <password> <MAC Address>
                // args          0             1           2           3       
                if(args.size() != 4) {
                    Serial_command_info_t info = Serial_command_info[SERIAL_COMMAND_NVS_REGISTER_USER];
                    printf("Error: %s %s\n", info.command.c_str(), info.format.c_str());
                    break;
                }
                else {
                    mac_addr_t mac;
                    if(mac_addr_parsing(args[3], mac) == ESP_FAIL) {
                        printf("MAC Address was invalid\n");
                        break;
                    }

                    //saving nvs
                    std::scoped_lock<std::shared_mutex> lk(nvs_mutex);
                    ESP_NVS user_nvs(args[1]);
                    user_nvs.open();
                    user_nvs.write("pw", args[2]);
                    user_nvs.write("mac", mac.data);
                    user_nvs.commit();

                    #ifdef DEBUG
                    mac_addr_t mac2;
                    user_nvs.read("mac", &mac2.data);
                    printf("NVS Read Check MAC: %x-%x-%x-%x-%x-%x, mac.data: %lld\n",
                        mac2.addr[0],
                        mac2.addr[1],
                        mac2.addr[2],
                        mac2.addr[3],
                        mac2.addr[4],
                        mac2.addr[5],
                        mac2.data
                    );
                    #endif
                    user_nvs.close();
                    printf("Please /reboot\n");
                }
                break;
            
            case SERIAL_COMMAND_NVS_SHOW_USER:
                //nvs_show_user <Username>
                {
                    std::scoped_lock<std::shared_mutex> lk(nvs_mutex);
                    ESP_NVS user_nvs(args[1]);
                    esp_err_t err = user_nvs.open(NVS_READONLY);
                    if(err == ESP_ERR_NVS_NOT_FOUND) printf("Unregistered Username\n");
                    else if (err != ESP_OK) ESP_LOGI(NVS_TAG, "Error (%s)", esp_err_to_name(err));

                    char pw[30];
                    mac_addr_t mac;
                    user_nvs.read("pw", pw);
                    user_nvs.read("mac", &mac.data);
                    printf("pw: %s, MAC: %x-%x-%x-%x-%x-%x\n",
                        pw,
                        mac.addr[0],
                        mac.addr[1],
                        mac.addr[2],
                        mac.addr[3],
                        mac.addr[4],
                        mac.addr[5]
                        );
                        user_nvs.close();
                }
                break;
    }
}