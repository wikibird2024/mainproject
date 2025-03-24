#include <stdio.h>
#include "comm.h"
#include "buzzer.h"
#include "mpu6050.h"
#include "sim4g_gps.h"

void app_main(void) {
    printf("🚀 ESP32 Fall Alert System Started\n");
    
    comm_init();
    buzzer_init();
    mpu6050_init();
    sim4g_gps_init();
}
