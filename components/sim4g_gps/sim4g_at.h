
#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Send a raw AT command and receive response.
 *
 * @param[in]  cmd       The AT command to send (must include "\r\n").
 * @param[out] resp_buf  Buffer to receive response (can be NULL if not needed).
 * @param[in]  len       Size of response buffer.
 * @param[in]  timeout_ms Timeout to wait for response in milliseconds.
 * @return true if a response was received and contains data.
 */
bool sim4g_send_cmd(const char *cmd, char *resp_buf, size_t len,
                    int timeout_ms);

/**
 * @brief Send AT command and check if expected string is in response.
 *
 * @param[in] cmd     Command string (must end with \r\n).
 * @param[in] expect  Expected substring in response.
 * @return true if expected substring found.
 */
bool sim4g_send_and_wait_prompt(const char *cmd, const char *expect);

/**
 * @brief Flush UART receive buffer before sending AT command.
 */
void sim4g_flush_uart(void);

#ifdef __cplusplus
}
#endif
