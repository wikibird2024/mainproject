idf_component_register(SRCS "src/fall_logic.c"
                      INCLUDE_DIRS "include"
                      REQUIRES data_manager log
                      PRIV_REQUIRES freertos mpu6050 debugs event_handler)


