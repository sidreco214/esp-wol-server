#ifndef _UDP_CLIENT_H_
#define _UDP_CLIENT_H_

#include <sys/param.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"

#define UDP_CLIENT_TAG "UDP Client"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char* dest_ip_addr;
    unsigned int dest_port;
    int addr_family;
    int ip_protocol;
    uint timeout_sec;
    char* message;
} udp_client_send_param_t;

esp_err_t udp_client_send(udp_client_send_param_t* param);

#ifdef __cplusplus
}
#endif

#endif