
#include "sim4g_at.h"
#include "comm_uart.h"
#include "debugs.h"
#include <stdio.h>
#include <string.h>

#define TAG "SIM4G_AT"
#define DEFAULT_CMD_TIMEOUT_MS 1000

bool sim4g_send_cmd(const char *cmd, char *resp_buf, size_t len,
                    int timeout_ms) {
  if (!cmd) {
    debugs_log_error(TAG, "Null AT command pointer");
    return false;
  }

  debugs_log_debug(TAG, "Sending: %s", cmd);

  // Flush UART RX buffer before sending
  comm_uart_flush();

  // Send command string (ends with \r\n)
  if (!comm_uart_send_command(cmd)) {
    debugs_log_error(TAG, "UART send failed");
    return false;
  }

  // If response buffer is NULL, don't wait for response
  if (!resp_buf || len == 0) {
    return true;
  }

  memset(resp_buf, 0, len);

  // Receive response with timeout
  if (!comm_uart_receive_response(resp_buf, len, timeout_ms)) {
    debugs_log_warn(TAG, "No response or timeout");
    return false;
  }

  debugs_log_debug(TAG, "Response: %s", resp_buf);
  return true;
}

bool sim4g_send_and_wait_prompt(const char *cmd, const char *expect) {
  char buffer[256];
  if (!sim4g_send_cmd(cmd, buffer, sizeof(buffer), DEFAULT_CMD_TIMEOUT_MS)) {
    return false;
  }

  return (strstr(buffer, expect) != NULL);
}

void sim4g_flush_uart(void) { comm_uart_flush(); }
