idf_component_register(SRCS "src/user_mqtt.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "mqtt" "data_manager" "json_wrapper" "log")
