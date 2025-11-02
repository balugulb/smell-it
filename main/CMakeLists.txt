idf_component_register(
    SRCS "gas_sensor.cpp" "wifi_manager.c" "main.cpp" "tcp_server.c" "display.cpp" "deepsleep.c" "touch.c" "gas_sensor.cpp"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash adafruit_tft mq2
)
