
idf_component_register(
    SRCS 
        "main.c"
        "app_main.c"
        "event_handler.c"
    INCLUDE_DIRS "."
    REQUIRES mqtt
    PRIV_REQUIRES 
        buzzer 
        comm 
        debugs 
        led_indicator 
        mpu6050 
        sim4g_gps 
        fall_logic 
        wifi_connect
)

# Đăng ký file Kconfig nếu cần tùy chỉnh
set(KCONFIG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig)
