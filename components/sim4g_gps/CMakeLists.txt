idf_component_register(
    SRCS 
        "src/sim4g_gps.c"
        "src/sim4g_at.c"
    INCLUDE_DIRS 
        "include"
    PRIV_INCLUDE_DIRS 
        "src"
    PRIV_REQUIRES 
        driver freertos log comm data_manager
)
