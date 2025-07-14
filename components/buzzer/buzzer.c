/**
 * @file buzzer.c
 * @brief Implementation of active buzzer control using PWM.
 */

#include "buzzer.h"
#include "comm.h"
#include "debugs.h"

#define BUZZER_PWM_PIN     18     // GPIO connected to active buzzer
#define BUZZER_PWM_FREQ    2000   // 2 kHz tone
#define BUZZER_DUTY_CYCLE  50     // 50% duty cycle for audible tone

static bool buzzer_initialized = false;

esp_err_t buzzer_init(void)
{
    if (buzzer_initialized) {
        DEBUGS_LOGI("Buzzer already initialized.");
        return ESP_OK;
    }

    esp_err_t err = comm_pwm_init(BUZZER_PWM_PIN, BUZZER_PWM_FREQ);
    if (err != ESP_OK) {
        DEBUGS_LOGE("PWM init failed for buzzer: %s", esp_err_to_name(err));
        return err;
    }

    buzzer_initialized = true;
    DEBUGS_LOGI("Buzzer initialized on GPIO %d at %d Hz", BUZZER_PWM_PIN, BUZZER_PWM_FREQ);
    return ESP_OK;
}

esp_err_t buzzer_on(void)
{
    if (!buzzer_initialized) {
        DEBUGS_LOGW("Buzzer not initialized.");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = comm_pwm_set_duty_cycle(BUZZER_DUTY_CYCLE);
    if (err != ESP_OK) {
        DEBUGS_LOGE("Failed to turn buzzer ON: %s", esp_err_to_name(err));
        return err;
    }

    DEBUGS_LOGI("Buzzer ON");
    return ESP_OK;
}

esp_err_t buzzer_off(void)
{
    if (!buzzer_initialized) {
        DEBUGS_LOGW("Buzzer not initialized.");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = comm_pwm_stop();
    if (err != ESP_OK) {
        DEBUGS_LOGE("Failed to turn buzzer OFF: %s", esp_err_to_name(err));
        return err;
    }

    DEBUGS_LOGI("Buzzer OFF");
    return ESP_OK;
}
