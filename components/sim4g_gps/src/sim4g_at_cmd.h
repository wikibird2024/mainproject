/**
 * @file sim4g_at_cmd.h
 * @brief AT command definitions for EC800K 4G-GPS module
 *
 * This file centralizes all AT commands used to interact with the EC800K
 * module. The goal is to improve maintainability, readability, and modularity.
 */

#pragma once

// General commands
#define AT_CMD_TEST "AT"
#define AT_CMD_ECHO_OFF "ATE0"
#define AT_CMD_CHECK_SIM "AT+CPIN?"
#define AT_CMD_SIGNAL_QUALITY "AT+CSQ"

// GPS-related commands
#define AT_CMD_GPS_CHECK "AT+CGNSINF\r\n"
#define AT_CMD_GPS_ENABLE "AT+QGPS=1"
#define AT_CMD_GPS_DISABLE "AT+QGPSEND"
#define AT_CMD_GPS_STATUS "AT+QGPS?"
#define AT_CMD_GPS_LOCATION "AT+QGPSLOC=2"

// GPS extension used in sim4g_at.c
#define AT_CMD_GPS_AUTOSTART "AT+CGPSAUTOSTART=1\r\n"
#define AT_CMD_GPS_ON "AT+CGPS=1\r\n"
#define AT_CMD_GPS_LOC_QUERY "AT+CGPSINFO\r\n"

// SMS-related commands
#define AT_CMD_SMS_MODE_TEXT "AT+CMGF=1"
#define AT_CMD_SET_PHONE_FMT "AT+CSCS=\"GSM\""
#define AT_CMD_SEND_SMS_PREFIX "AT+CMGS=\"" // Usage: AT+CMGS="phone"\r
#define AT_CMD_SMS_SEND_FMT "AT+CMGS=\"%s\"\r\n"
#define AT_CMD_SMS_CTRL_Z "\x1A"

// Network-related
#define AT_CMD_GET_OPERATOR "AT+COPS?"
#define AT_CMD_GET_NETWORK_TYPE "AT+QNWINFO"
#define AT_CMD_ATTACH_STATUS "AT+CGATT?"

// Others
#define AT_CMD_SAVE_CFG "AT&W"
