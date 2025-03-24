#include "sim4g_gps.h"
#include "comm.h"
#include "debugs.h"
#include <string.h>

#define DEFAULT_PHONE "+84901234567"

static char phone_number[15] = DEFAULT_PHONE;

void sim4g_gps_init() {
    comm_send_command("AT+QGPS=1");
    debugs_log("SIM4G_GPS", "GPS initialized.");
}

void sim4g_gps_set_phone_number(const char *number) {
    if (number && strlen(number) < sizeof(phone_number)) {
        strncpy(phone_number, number, sizeof(phone_number) - 1);
        phone_number[sizeof(phone_number) - 1] = '\0';
        debugs_log("SIM4G_GPS", "Phone set: %s", phone_number);
    }
}

gps_data_t sim4g_gps_get_location() {
    gps_data_t location = {0};
    comm_send_command("AT+QGPSLOC?");
    
    strcpy(location.latitude, "10.762622");
    strcpy(location.longitude, "106.660172");
    strcpy(location.timestamp, "120320,153045");

    debugs_log("SIM4G_GPS", "Location: %s, %s at %s",
               location.latitude, location.longitude, location.timestamp);
    return location;
}

void send_fall_alert_sms(gps_data_t location) {
    char sms[100];
    snprintf(sms, sizeof(sms), "Fall detected! Location: %s, %s",
             location.latitude, location.longitude);

    comm_send_command("AT+CMGS=\"%s\"", phone_number);
    comm_send_command(sms);

    debugs_log("SIM4G_GPS", "SMS sent: %s", sms);
}

