idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES mpu6050 buzzer sim4g_gps comm debugs
)
