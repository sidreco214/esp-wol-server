#include "udp_client.h"

esp_err_t udp_client_send(udp_client_send_param_t *param) {
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(param->dest_ip_addr);
    dest_addr.sin_family = param->addr_family;
    dest_addr.sin_port = htons(param->dest_port);

    int sock = socket(param->addr_family, SOCK_DGRAM, param->ip_protocol);
    if(sock < 0) {
        ESP_LOGE(UDP_CLIENT_TAG, "Unable to create socket: errno %d", errno);
        return ESP_FAIL;
    }

    //Set timeout
    struct timeval timeout;
    timeout.tv_sec = param->timeout_sec;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    ESP_LOGI(UDP_CLIENT_TAG, "Socket created, sending to %s:%d", param->dest_ip_addr, param->dest_port);

    while(true) {
        esp_err_t err = sendto(sock, param->message, strlen(param->message), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
        if(err < 0) {
            ESP_LOGE(UDP_CLIENT_TAG, "Error occurred during sending: errno %d", errno);
            break;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        if(sock != -1) {
            ESP_LOGI(UDP_CLIENT_TAG, "Success, shutdown and close socket");
            shutdown(sock, 0);
            close(sock);
            break;
        }
    }
    return ESP_OK;
}
