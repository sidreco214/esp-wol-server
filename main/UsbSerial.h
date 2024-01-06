#ifndef USBSERIAL
#define USEBSERIAL

#include <stdio.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "driver/gpio.h"
#include "string.h"
#include "stdarg.h"

#define UART0_RX_BUF_SIZE 256

#ifdef __cplusplus
extern "C" {
#endif

void uart0_init(uint32_t baud);
int uart0_sendData(const char* data); //보낸 바이트 수를 리턴
void uart0_printf(const char* Format, ...);
int uart0_readData(char* buffer); //읽은 바이트 수 리턴, 없으면 0 리턴

#ifdef __cplusplus
}
#endif

#endif