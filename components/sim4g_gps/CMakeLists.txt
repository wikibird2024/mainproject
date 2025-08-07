idf_component_register(
    SRCS 
        "src/sim4g_gps.c"
        "src/sim4g_at.c"
    INCLUDE_DIRS 
        "include"
    PRIV_INCLUDE_DIRS 
        "src"
    REQUIRES 
        comm user_mqtt data_manager freertos log driver
    PRIV_REQUIRES 
        esp_timer 
)
