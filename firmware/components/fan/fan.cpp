#include "fan.hpp"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "hal/gpio_types.h"
#include "soc/gpio_num.h"
#include <stdio.h>

#ifndef ESP_RETURN_ON_ERROR
#define ESP_RETURN_ON_ERROR(err, tag, msg)                                     \
  do {                                                                         \
    esp_err_t __err = (err);                                                   \
    if (__err != ESP_OK) {                                                     \
      ESP_LOGE(tag, "%s: %s", msg, esp_err_to_name(__err));                    \
      return __err;                                                            \
    }                                                                          \
  } while (0)
#endif

static const char *TAG = "PWMFan";

const int kMaxRPM = 10000;
const int kMinRPM = 0;

/**
 * Util function for calculating exponential moving average
 */
inline float ema(float value, float alpha, float prev) {
  return alpha * value + (1 - alpha) * prev;
}

/**
 * Handles ISR for tachometer pin
 */
void IRAM_ATTR PWMFan::_on_tach_interrupt(void *arg) {
  auto *fan = static_cast<PWMFan *>(arg);
  int64_t currentTime = esp_timer_get_time();

  int64_t diff = currentTime - fan->_lastInterruptTime;

  // 10000us = 10ms
  // no way our fan is going >3k RPM, so ignore
  // rated for 1550 RPM max.
  if (diff < 10'000) {
    return;
  }

  fan->_lastPulsePeriod = diff;
  fan->_lastInterruptTime = currentTime;
}

/**
 * Constructor
 */
esp_err_t PWMFan::init() {
  // initialize PWM
  ledc_channel_config_t channel_conf = {
      .gpio_num = this->_config.pwm_pin,
      .speed_mode = this->_config.pwm_mode,
      .channel = this->_config.pwm_channel,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = this->_config.pwm_timer,
      .duty = 0,
      .hpoint = 0,
      .flags = {
          .output_invert = 0 // Standard: 0V = Stop, 3.3V (Float) = Run
      }};

  ESP_RETURN_ON_ERROR(ledc_channel_config(&channel_conf), TAG,
                      "Failed to configure PWM channel");
  ESP_LOGI(TAG, "PWM channel initialized on pin %d", this->_config.pwm_pin);

  // ESP_RETURN_ON_ERROR(
  //     gpio_set_drive_capability(this->_config.pwm_pin, GPIO_DRIVE_CAP_3),
  //     TAG, "Failed to set driver cap for fan");

  // ESP_RETURN_ON_ERROR(
  //     gpio_set_direction(this->_config.pwm_pin, GPIO_MODE_OUTPUT), TAG,
  //     "Failed to set GPIO mode for fan");

  // initialize tachometer if provided
  if (this->_config.tach_pin != GPIO_NUM_NC) {
    gpio_config_t tach_config = {.pin_bit_mask =
                                     (1ULL << this->_config.tach_pin),
                                 .mode = GPIO_MODE_INPUT,
                                 .pull_up_en = GPIO_PULLUP_ENABLE,
                                 .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                 .intr_type = GPIO_INTR_NEGEDGE};

    ESP_RETURN_ON_ERROR(gpio_config(&tach_config), TAG,
                        "Failed to configure tachometer pin");

    _lastInterruptTime = esp_timer_get_time();
    gpio_isr_handler_add(this->_config.tach_pin, _on_tach_interrupt, this);
    ESP_LOGI(TAG, "Tachometer initialized on pin %d", this->_config.tach_pin);
  }

  // initialize enable pin if provided
  if (this->_config.enable_pin != GPIO_NUM_NC) {
    gpio_config_t enable_config = {.pin_bit_mask =
                                       (1ULL << this->_config.enable_pin),
                                   .mode = GPIO_MODE_OUTPUT,
                                   .pull_up_en = GPIO_PULLUP_DISABLE,
                                   .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                   .intr_type = GPIO_INTR_DISABLE};

    ESP_RETURN_ON_ERROR(gpio_config(&enable_config), TAG,
                        "Failed to configure enable pin");

    gpio_set_level(this->_config.enable_pin, 1);
    ESP_LOGI(TAG, "Enable pin initialized on pin %d", this->_config.enable_pin);
  }

  return ESP_OK;
}

void PWMFan::setSpeed(int speed) {
  ledc_set_duty(this->_config.pwm_mode, this->_config.pwm_channel,
                speed * this->_resolution_max_duty / 100);

  ledc_update_duty(this->_config.pwm_mode, this->_config.pwm_channel);
}

void PWMFan::setEnabled(bool enabled) {
  if (this->_config.enable_pin != GPIO_NUM_NC) {
    gpio_set_level(this->_config.enable_pin, enabled ? 1 : 0);

    // set pin floating if disabled
    if (enabled) {
      gpio_set_direction(this->_config.enable_pin, GPIO_MODE_OUTPUT);
      gpio_set_direction(this->_config.tach_pin, GPIO_MODE_INPUT);
    } else {
      gpio_set_direction(this->_config.enable_pin, GPIO_MODE_DISABLE);
      gpio_set_direction(this->_config.tach_pin, GPIO_MODE_DISABLE);
    }
  }

  // log if changed
  if (this->_enabled != enabled) {
    ESP_LOGI(TAG, "Set fan enable pin %d to %d", this->_config.enable_pin,
             enabled ? 1 : 0);
  }

  this->_enabled = enabled;
}

void PWMFan::toggle() { this->setEnabled(!this->_enabled); }
float PWMFan::getRPM() {
  if (_lastPulsePeriod == 0)
    return 0;

  int64_t now = esp_timer_get_time();

  // If no pulse for 1 second (1,000,000 us), assume 0 RPM
  if ((now - _lastInterruptTime) > 1000000) {
    return _rpm = 0;
  }

  // Instantaneous RPM
  // 2 pulses per revolution
  float currentRpm = (60000000.0f / (float)_lastPulsePeriod) / 2.0f;

  return this->_rpm = ema(currentRpm, 0.1, this->_rpm);
}
