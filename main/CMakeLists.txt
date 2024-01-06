idf_component_register(SRCS "esp-wol-server.cpp" "Wifi.c" "UsbSerial.c" "udp_client.c" "util.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES esp_driver_gpio nvs_flash esp_wifi esp_https_server esp_driver_uart
                    EMBED_TXTFILES "certs/servercert.pem" "certs/prvtkey.pem" "html/root_get.html"
                    )
