idf_component_register(
    SRCS "sim4g_gps.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES driver esp_system comm debugs
)
