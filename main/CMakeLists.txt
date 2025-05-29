idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES  buzzer comm debugs led_indicator mpu6050 sim4g_gps
)
