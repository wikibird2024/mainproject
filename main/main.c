/**
 * @file main.c
 * @brief Entry point for the ESP32 Fall Detection System.
 *
 * Delegates system startup to app_main.c.
 */

#include "app_main.h"

void app_main(void) {
  app_system_init();       // Orchestrate system init

  app_start_application(); // Start tasks or business logic
}
