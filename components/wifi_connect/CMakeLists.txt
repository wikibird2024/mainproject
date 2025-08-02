
idf_component_register(
    SRCS "src/wifi_connect.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_event nvs_flash freertos debugs
)
