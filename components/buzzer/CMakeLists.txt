idf_component_register(
    SRCS "src/buzzer.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES debugs comm log freertos driver
)

