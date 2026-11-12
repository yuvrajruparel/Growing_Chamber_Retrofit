#pragma once
#include "esp_attr.h"
#include "esp_err.h"
#include "hal/ledc_types.h"
#include "soc/gpio_num.h"
#include <cstdint>
#include <functional>
#include <memory>

class PWMFan;

struct PWMFanConfig {
  gpio_num_t pwm_pin;
  ledc_channel_t pwm_channel;
  ledc_timer_t pwm_timer;

  int pwm_resolution = LEDC_TIMER_10_BIT;
  ledc_mode_t pwm_mode = LEDC_LOW_SPEED_MODE;

  gpio_num_t enable_pin = GPIO_NUM_NC;
  gpio_num_t tach_pin = GPIO_NUM_NC;
};

struct PWMFanWDGEntry {
  std::shared_ptr<PWMFan> fan;
  gpio_num_t fault_pin;
};

/**
 * PWM 4-pin or 3-pin fan controller with (optionally) an external MOSFET enable
 * pin.
 */
class PWMFan {
private:
  // configuration variables
  PWMFanConfig _config;
  std::function<void *(void *)> _on_failure_callback;
  int _resolution_max_duty = 1023;

  // running variables
  bool _enabled = false;
  int _speed = 0;
  float _rpm = 0;

  // used in interrupt
  volatile int64_t _lastPulsePeriod = 0;
  volatile int64_t _lastInterruptTime = 0;

  /**
   * Handles ISR for tachometer pin
   */
  static void IRAM_ATTR _on_tach_interrupt(void *arg);

  static std::vector<PWMFanWDGEntry> _wdg_fans;

  /**
   * Fan watchdog task to monitor fan health
   */
  static void _wdgTask(void *arg);

public:
  /**
   * Must configure the PWM timer before initializing the fan!
   *
   * ```
   *   ledc_timer_config_t timer_config = {
   *     .speed_mode = LEDC_HIGH_SPEED_MODE,
   *     .duty_resolution = LEDC_TIMER_10_BIT,
   *     .timer_num = LEDC_TIMER_0,
   *     .freq_hz = 5000,
   *     .clk_cfg = LEDC_AUTO_CLK
   *   };
   *   ledc_timer_config(&timer_config);
   * ```
   */
  PWMFan(PWMFanConfig config)
      : _config(config), _on_failure_callback(nullptr) {}

  esp_err_t init();

  void setEnabled(bool enabled);
  void toggle();

  void setSpeed(int speed);
  float getRPM();

  /**
   * Registers a fan to be monitored by the watchdog task.
   */
  static esp_err_t wdgRegisterFan(std::shared_ptr<PWMFan> fan,
                                  gpio_num_t faultPin);

  /**
   * Starts the fan watchdog task. Register fans using PWMFan::wdgRegisterFan(),
   * and this task will check their RPM periodically.
   */
  static void wdgStart();
};
