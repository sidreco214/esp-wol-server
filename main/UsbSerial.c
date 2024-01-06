#include "UsbSerial.h"

void uart0_init(uint32_t baud) {
    const uart_config_t uart_config = {
        .baud_rate = baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_0, UART0_RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, GPIO_NUM_0, GPIO_NUM_3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
};

int uart0_sendData(const char *data) {
    const int len = strlen(data);
    const int sendBytes = uart_write_bytes(UART_NUM_0, data, len);
    printf("Worte %d Bytes: '%s'\n", sendBytes, data);
    return sendBytes;
}
void uart0_printf(const char *Format, ...) {
    char buf[256];
    va_list args;
    va_start(args, Format);
    vsnprintf(buf, 256, Format, args);
    uart0_sendData((char*) buf);
    va_end(args);
};

int uart0_readData(char* buffer) {
    const int readBytes = uart_read_bytes(UART_NUM_0, buffer, UART0_RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
    if(readBytes > 0) {
        buffer[readBytes] = 0;
        printf("UART0 Read %d Bytes: '%s'\n", readBytes, buffer);
    }
    return readBytes;
};
