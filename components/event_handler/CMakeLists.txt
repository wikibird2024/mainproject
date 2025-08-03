
idf_component_register(
    SRCS "src/event_handler.c"
    INCLUDE_DIRS "include"
    REQUIRES fall_logic buzzer led_indicator sim4g_gps debugs
)
