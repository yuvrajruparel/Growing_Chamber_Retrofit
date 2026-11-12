#pragma once

#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_timer.h"
#include "soc/gpio_num.h"
#include <functional>

class WaterLevelSensor {
public:
  /**
   * Callback signature: void(bool isWet)
   * isWet: true if water is detected, false if dry
   */
  using SensorCallback = std::function<void(bool isWet)>;

  /**
   * @param pin The GPIO pin connected to the sensor signal
   * @param debounceMs Time in milliseconds to wait for signal stability
   * (default 50ms)
   */
  WaterLevelSensor(gpio_num_t pin, uint64_t debounceMs = 50);
  ~WaterLevelSensor();

  /**
   * Registers the function to be called on state change.
   */
  void setCallback(SensorCallback cb);

  /**
   * Configures GPIO, Timer, and ISR.
   * Call this after registering the callback.
   * Note: Requires gpio_install_isr_service(0) to be called in app_main
   * previously.
   */
  void begin();

private:
  gpio_num_t pin_;
  uint64_t debounceDelayUs_;
  SensorCallback callback_;
  esp_timer_handle_t timerHandle_;

  // Static ISR wrapper (executes in IRAM)
  static void IRAM_ATTR isrHandler(void *arg);

  // Static Timer wrapper (executes in High Priority Task)
  static void timerHandler(void *arg);
};
