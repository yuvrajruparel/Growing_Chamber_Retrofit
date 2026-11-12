#include "driver/gpio.h"
#include "esp_log.h"
#include "fan.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "hal/gpio_types.h"
#include "soc/gpio_num.h"

static const char *TAG = "FanWatchdog";

std::vector<PWMFanWDGEntry> PWMFan::_wdg_fans;

esp_err_t PWMFan::wdgRegisterFan(std::shared_ptr<PWMFan> fan,
                                 gpio_num_t faultPin) {
  PWMFan::_wdg_fans.push_back(
      PWMFanWDGEntry{.fan = std::move(fan), .fault_pin = faultPin});

  if (faultPin != GPIO_NUM_NC) {
    // Initialize fault pin
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << faultPin);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    return gpio_config(&io_conf);
  }

  return ESP_OK;
}

void PWMFan::_wdgTask(void *args) {
  const int checkIntervalMs = 2500;
  const int rpmThreshold = 50; // RPM below which fan is considered failed

  while (true) {
    for (const auto &entry : PWMFan::_wdg_fans) {
      float currentRPM = entry.fan->getRPM();
      if (entry.fan->_enabled && currentRPM < rpmThreshold) {
        ESP_LOGE(TAG, "Fan on pin %d failed! RPM: %f",
                 entry.fan->_config.pwm_pin, currentRPM);

        // Set fault pin high to indicate failure
        if (entry.fault_pin != GPIO_NUM_NC) {
          gpio_set_level(entry.fault_pin, 1);
        }
      } else {
        // Clear fault pin if fan is healthy
        if (entry.fault_pin != GPIO_NUM_NC) {
          gpio_set_level(entry.fault_pin, 0);
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(checkIntervalMs));
  }
}

void PWMFan::wdgStart() {
  xTaskCreate(_wdgTask, "FanWatchdogTask", 4096, nullptr, 5, nullptr);
}
