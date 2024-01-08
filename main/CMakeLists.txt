set(CMAKE_C_FLAGS_DEBUG "-DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG")

idf_component_register(SRCS "esp-wol-server.cpp" "Wifi.c" "UsbSerial.c" "udp_client.c" "util.cpp" "spiffs.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_driver_gpio nvs_flash esp_wifi esp_https_server esp_driver_uart spiffs
                    EMBED_TXTFILES "html/root_get.html"
                    )

spiffs_create_partition_image(storage ../certs FLASH_IN_PROJECT)