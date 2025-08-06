idf_component_register(SRCS "src/data_manager.c"
                    INCLUDE_DIRS "include"
                    REQUIRES freertos log sim4g_gps
                    PRIV_REQUIRES esp_timer event_handler)
