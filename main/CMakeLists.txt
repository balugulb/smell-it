idf_component_register(
    SRCS "wifi_manager.c" "main.cpp" "tcp_server.c" "display.cpp" "variables.cpp" "deepsleep.c" "touch.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash adafruit_tft 
)
