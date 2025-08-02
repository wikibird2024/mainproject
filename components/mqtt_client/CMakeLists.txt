
idf_component_register(
    SRCS "src/mqtt_client.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_mqtt_idf cjson debugs 
)
