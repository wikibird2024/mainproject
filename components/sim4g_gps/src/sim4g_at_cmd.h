/**
 * @file sim4g_at_cmd.h
 * @brief Centralized AT command definitions for EC800K 4G-GPS module
 *
 * Compliant with Quectel EC800K/EG800K Series documentation.
 * Uses a data-driven approach for improved readability and scalability.
 */

#pragma once

#include <stdint.h>

// -----------------------------------------------------------
// Enumeration of AT Command IDs
// -----------------------------------------------------------
typedef enum {
    AT_CMD_TEST_ID,
    AT_CMD_ECHO_OFF_ID,
    AT_CMD_SAVE_CFG_ID,
    AT_CMD_GET_MODULE_INFO_ID,
    AT_CMD_GET_IMEI_ID,

    AT_CMD_CHECK_SIM_ID,
    AT_CMD_GET_IMSI_ID,
    AT_CMD_GET_ICCID_ID,
    AT_CMD_GET_SMS_MODE_ID,
    AT_CMD_GET_CHARSET_ID,

    AT_CMD_SIGNAL_QUALITY_ID,
    AT_CMD_GET_OPERATOR_ID,
    AT_CMD_GET_NETWORK_TYPE_ID,
    AT_CMD_ATTACH_STATUS_ID,
    AT_CMD_REGISTRATION_STATUS_ID,

    AT_CMD_SMS_MODE_TEXT_ID,
    AT_CMD_SET_CHARSET_GSM_ID,
    AT_CMD_SEND_SMS_PREFIX_ID,
    AT_CMD_SMS_CTRL_Z_ID,

    AT_CMD_ANSWER_CALL_ID,
    AT_CMD_HANGUP_CALL_ID,

    AT_CMD_GPS_ENABLE_ID,
    AT_CMD_GPS_DISABLE_ID,
    AT_CMD_GPS_STATUS_ID,
    AT_CMD_GPS_LOCATION_ID,
    
    AT_CMD_GPS_AUTOGPS_ON_ID,
    AT_CMD_GPS_OUTPORT_USB_ID,
    AT_CMD_GPS_XTRA_ENABLE_ID,
    AT_CMD_GPS_UTC_TIME_ID,

    AT_CMD_CELL_LOCATE_ID,

    AT_CMD_MAX_COUNT, // Used to get the total number of commands
} at_cmd_id_t;


// -----------------------------------------------------------
// Data structure for an AT command
// -----------------------------------------------------------
typedef struct {
    at_cmd_id_t id;
    const char *cmd_string;
    uint32_t timeout_ms;
} at_command_t;


// -----------------------------------------------------------
// Global AT Command Table
// -----------------------------------------------------------
extern const at_command_t at_command_table[AT_CMD_MAX_COUNT];

// -----------------------------------------------------------
// End of sim4g_at_cmd.h
// -----------------------------------------------------------
