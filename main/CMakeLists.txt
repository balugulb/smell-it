idf_component_register(
    SRCS "wifi_manager.cpp" "main.cpp" "tcp_server.cpp" "display.cpp" "variables.cpp" "deepsleep.cpp"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash adafruit_tft touchsensor
)
