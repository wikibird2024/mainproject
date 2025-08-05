
idf_component_register(
    SRCS "src/my_mqtt_client.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_mqtt_idf common json_wrapper debugs 
)
