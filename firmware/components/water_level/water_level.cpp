#include "include/water_level.hpp"
#include "esp_attr.h"
#include "hal/gpio_types.h"
#include "soc/gpio_num.h"
#include <cstdint>

WaterLevelSensor::WaterLevelSensor(gpio_num_t pin, uint64_t debounceMs)
    : pin_(pin), debounceDelayUs_(debounceMs * 1000), callback_(nullptr),
      timerHandle_(nullptr) {}

WaterLevelSensor::~WaterLevelSensor() {
  gpio_isr_handler_remove(pin_);
  if (timerHandle_) {
    esp_timer_stop(timerHandle_);
    esp_timer_delete(timerHandle_);
  }
}

void WaterLevelSensor::setCallback(SensorCallback cb) { callback_ = cb; }

void WaterLevelSensor::begin() {
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_ANYEDGE;
  io_conf.pin_bit_mask = (1ULL << pin_);
  io_conf.mode = GPIO_MODE_INPUT;

  // Disable internal pulls (User has external voltage divider)
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

  gpio_config(&io_conf);

  // 2. Create Debounce Timer
  esp_timer_create_args_t timer_args = {};
  timer_args.callback = &WaterLevelSensor::timerHandler;
  timer_args.arg = this; // Pass instance to static handler
  timer_args.name = "water_debounce";

  esp_timer_create(&timer_args, &timerHandle_);

  // 3. Add ISR Handler
  // Note: Assumes gpio_install_isr_service(0) is called in main
  gpio_isr_handler_add(pin_, isrHandler, this);
}

// IRAM_ATTR is crucial here as this runs in interrupt context
void IRAM_ATTR WaterLevelSensor::isrHandler(void *arg) {
  auto *self = static_cast<WaterLevelSensor *>(arg);

  // Restart the timer on every edge.
  // The callback only fires if the signal stays stable for debounceDelayUs_.
  if (self->timerHandle_) {
    esp_timer_start_once(self->timerHandle_, self->debounceDelayUs_);
  }
}

void WaterLevelSensor::timerHandler(void *arg) {
  auto *self = static_cast<WaterLevelSensor *>(arg);

  if (self->callback_) {
    // Read the actual GPIO level after the debounce period
    // 1 = Wet (High), 0 = Dry (Low)
    bool isWet = (gpio_get_level(self->pin_) == 1);
    self->callback_(isWet);
  }
}
