#include "Wifi.h"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define STA_TAG "Wifi Station"

static bool wifi_is_connected = false;
static int maximun_retry_connecting = 0;
static esp_netif_ip_info_t ip_info;
static uint8_t base_mac_addr[6] = {0};

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    static int num_retry = 0;

    switch (event_id) {
    case WIFI_EVENT_STA_START:
        ESP_LOGI(STA_TAG, "Try connecting WIFI");
        if(event_base == WIFI_EVENT) esp_wifi_connect();
        break;
    case WIFI_EVENT_STA_CONNECTED:
        wifi_is_connected = true;
        ESP_LOGI(STA_TAG, "WIFI is connected");
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        if(event_base == WIFI_EVENT && num_retry < maximun_retry_connecting) {
            wifi_is_connected = false;
            esp_wifi_connect();
            ++num_retry;
            ESP_LOGI(STA_TAG, "retry to connect to the AP");
        }
        else if(num_retry >= maximun_retry_connecting) {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGI(STA_TAG, "connect to the AP fail");
        }
        break;
    case IP_EVENT_STA_GOT_IP:
        if(event_base == IP_EVENT) {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            ESP_LOGI(STA_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            ip_info = event->ip_info;
            num_retry = 0;
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        }
        break;
    default:
        break;
    }
};

bool wifi_is_available() {
    return wifi_is_connected;
}

esp_err_t wifi_init_sta(const char* ssid, const char* password, int maximun_retry) {
    maximun_retry_connecting = maximun_retry;

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init()); //TCP
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id; //for all Wifi event
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    wifi_config_t wifi_config = {
        .sta = {
	        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strcpy((char*)wifi_config.sta.ssid, ssid); //ssid를 uint8_t로 정의해놔서 그냥은 안됨
    strcpy((char*)wifi_config.sta.password, password);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(STA_TAG, "wifi_init_sta finished.");

    ESP_ERROR_CHECK(esp_read_mac(base_mac_addr, ESP_MAC_EFUSE_FACTORY));

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    esp_err_t err;
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(STA_TAG, "connected to SSID:%s, IP:" IPSTR ", MAC Address:%x-%x-%x-%x-%x-%x",
            ssid,
            IP2STR(&ip_info.ip),
            base_mac_addr[0],
            base_mac_addr[1],
            base_mac_addr[2],
            base_mac_addr[3],
            base_mac_addr[4],
            base_mac_addr[5]
            );
        wifi_is_connected = true;
        err = ESP_OK;
    }
    else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(STA_TAG, "Failed to connect to SSID:%s", ssid);
        err = ESP_FAIL;
    }
    else {
        ESP_LOGE(STA_TAG, "UNEXPECTED EVENT");
        err = ESP_FAIL;
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
    return err;
}
