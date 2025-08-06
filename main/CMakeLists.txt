# main/CMakeLists.txt - Improved version

idf_component_register(
    SRCS 
        "main.c"
        "app_main.c"
    
    INCLUDE_DIRS 
        "."
    
    # Public dependencies - components whose headers are exposed to other components
    REQUIRES 
        freertos          # FreeRTOS is almost always needed
        esp_common        # Common ESP-IDF functionality
        log          # Logging functionality
        nvs_flash        # Non-volatile storage (if used)
        user_mqtt        # Your MQTT component (if headers needed by others)
        json_wrapper     # JSON handling (if headers needed by others)
        debugs           # Debug utilities (if headers needed by others)
        data_manager     # Moved to REQUIRES since other components depend on it
        comm 
    # Private dependencies - components used internally, headers not exposed
    PRIV_REQUIRES 
        buzzer 
        led_indicator 
        mpu6050 
        sim4g_gps 
        fall_logic 
        wifi_connect 
        event_handler
        # Remove data_manager from here since other components need its headers
)

# Optional: Set component-specific Kconfig if you have custom configurations
# Only uncomment if you actually have a Kconfig file in main directory
 set(COMPONENT_KCONFIG ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig)
