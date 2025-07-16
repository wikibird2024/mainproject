<<<<<<< HEAD
#pragma once

#include "esp_err.h"
#include <stdbool.h>
=======
#ifndef SIM4G_GPS_H
#define SIM4G_GPS_H
>>>>>>> 2c4b6e7 (add to rebase)

#ifdef __cplusplus
extern "C" {
#endif

<<<<<<< HEAD
typedef struct {
  float latitude;
  float longitude;
  float altitude;
  int utc_hour;
  int utc_min;
  int utc_sec;
  bool valid;
} gps_location_t;

/**
 * @brief Initialize SIM module (check UART, send AT).
 */
esp_err_t sim4g_init(void);

/**
 * @brief Enable GPS (cold start).
 */
esp_err_t sim4g_enable_gps(void);

/**
 * @brief Get GPS location.
=======
#include <stdint.h>
#include <stdbool.h>

/*--------------------------------------------------------------------------------------------------
 *  Module: SIM4G GPS Interface
 *  File:   sim4g_gps.h
 *  Author: [Your Name or Organization]
 *  Brief:  Provides interfaces for SIM4G GPS module operations, including GPS location retrieval
 *          and SMS transmission for fall alert systems.
 *------------------------------------------------------------------------------------------------*/

/*------------------------------------ Macros & Constants ----------------------------------------*/

#define SIM4G_GPS_PHONE_MAX_LEN        (20U)
#define SIM4G_GPS_STRING_MAX_LEN       (32U)

/*---------------------------------------- Enums -------------------------------------------------*/

/**
 * @enum sim4g_error_t
 * @brief Error codes returned by SIM4G GPS functions.
 */
typedef enum {
    SIM4G_SUCCESS = 0,
    SIM4G_ERROR_INVALID_PARAM,
    SIM4G_ERROR_MUTEX,
    SIM4G_ERROR_MEMORY,
    SIM4G_ERROR_COMMUNICATION,
    SIM4G_ERROR_TIMEOUT,
    SIM4G_ERROR_NETWORK
} sim4g_error_t;

/**
 * @enum comm_result_t
 * @brief Communication result status.
 */
typedef enum {
    COMM_SUCCESS = 0,
    COMM_ERROR,
    COMM_TIMEOUT
} comm_result_t;

/*-------------------------------------- Structures ----------------------------------------------*/

/**
 * @struct sim4g_gps_data_t
 * @brief Holds parsed GPS location data.
 */
typedef struct {
    char     timestamp[SIM4G_GPS_STRING_MAX_LEN];   /**< UTC Timestamp */
    char     latitude[SIM4G_GPS_STRING_MAX_LEN];    /**< Latitude value */
    char     lat_dir[2];                            /**< Latitude direction: "N" or "S" */
    char     longitude[SIM4G_GPS_STRING_MAX_LEN];   /**< Longitude value */
    char     lon_dir[2];                            /**< Longitude direction: "E" or "W" */
    uint8_t  valid;                                 /**< Validity flag: 1 = valid, 0 = invalid */
} sim4g_gps_data_t;

/**
 * @struct sim4g_gps_config_t
 * @brief Configuration parameters for SIM4G GPS module behavior.
 */
typedef struct {
    const char* default_phone_number;               /**< Default emergency phone number */
    uint32_t    cold_start_time_ms;                 /**< Cold start GPS timeout */
    uint32_t    normal_operation_time_ms;           /**< Regular GPS operation timeout */
    uint32_t    network_check_time_ms;              /**< Time to wait for network availability */
    uint32_t    network_check_interval_ms;          /**< Periodic check interval for network */
    uint8_t     min_signal_strength;                /**< Minimum acceptable RSSI */
    uint8_t     max_sms_retry_count;                /**< Retry count before giving up */
    size_t      response_buffer_size;               /**< UART/SIM buffer for AT command responses */
    uint32_t    retry_delay_ms;                     /**< Delay between retries */
    size_t      sms_buffer_size;                    /**< Size for storing outgoing SMS content */
    uint32_t    mutex_timeout_ms;                   /**< Mutex timeout for shared resources */
    uint32_t    gps_location_retry_interval_ms;     /**< Time between GPS location retry */
    uint32_t    sms_retry_interval_ms;              /**< Time between SMS send retries */
    uint32_t    sms_config_delay_ms;                /**< Delay after SMS configuration AT cmds */
    uint32_t    sms_recipient_delay_ms;             /**< Delay before setting SMS recipient */
    uint32_t    sms_message_delay_ms;               /**< Delay before sending SMS content */
    uint32_t    sms_post_send_delay_ms;             /**< Delay after sending SMS */
    uint32_t    sms_total_timeout_ms;               /**< Total timeout for sending an SMS */
    uint32_t    location_validity_ms;               /**< Validity time of last known location */
    uint8_t     max_sms_tasks;                      /**< Number of concurrent SMS tasks */
    uint8_t     gps_coord_min_length;               /**< Minimum length for valid GPS coord */
    uint8_t     gps_coord_max_length;               /**< Maximum length for valid GPS coord */
    uint16_t    sms_task_stack_size;                /**< Stack size for SMS task/thread */
    uint8_t     sms_task_priority;                  /**< Task priority for SMS operation */
} sim4g_gps_config_t;

/*------------------------------------- Function Typedefs ----------------------------------------*/

/**
 * @brief Callback prototype for asynchronous SMS transmission.
 *
 * @param success Indicates success (true) or failure (false) of the SMS operation.
 */
typedef void (*sms_callback_t)(bool success);

/*-------------------------------------- API Prototypes ------------------------------------------*/

/**
 * @brief Apply a new configuration to the SIM4G GPS module.
 *
 * @param new_cfg Pointer to the new configuration structure.
 */
void sim4g_set_config(const sim4g_gps_config_t* new_cfg);

/**
 * @brief Initialize the SIM4G GPS module with default timeouts.
>>>>>>> 2c4b6e7 (add to rebase)
 */
esp_err_t sim4g_get_location(gps_location_t *loc);

/**
<<<<<<< HEAD
 * @brief Send SMS with content to a phone number.
 */
esp_err_t sim4g_send_sms(const char *phone_number, const char *message);
=======
 * @brief Initialize the SIM4G GPS module with specified timeouts.
 *
 * @param cold_start_ms Cold start timeout in milliseconds.
 * @param normal_ms     Normal operation timeout.
 * @param network_ms    Network wait timeout.
 */
void sim4g_gps_init_with_timeout(uint32_t cold_start_ms, uint32_t normal_ms, uint32_t network_ms);

/**
 * @brief Set the phone number used for emergency SMS alerts.
 *
 * @param phone Pointer to the phone number string.
 * @return sim4g_error_t Error code indicating result.
 */
sim4g_error_t sim4g_gps_set_phone_number(const char *phone);

/**
 * @brief Retrieve the latest available GPS location.
 *
 * @return sim4g_gps_data_t Structure containing parsed GPS data.
 */
sim4g_gps_data_t sim4g_gps_get_location(void);

/**
 * @brief Asynchronously send a fall alert SMS containing GPS location.
 *
 * @param location Pointer to GPS location structure.
 * @param callback Callback function to handle result.
 * @return sim4g_error_t Error code indicating result.
 */
sim4g_error_t sim4g_gps_send_fall_alert_async(const sim4g_gps_data_t *location, sms_callback_t callback);
>>>>>>> 2c4b6e7 (add to rebase)

/*------------------------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif

<<<<<<< HEAD
=======
#endif /* SIM4G_GPS_H */
>>>>>>> 2c4b6e7 (add to rebase)
