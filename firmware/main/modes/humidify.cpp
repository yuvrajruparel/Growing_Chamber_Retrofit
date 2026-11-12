
#include <cstddef>

#include "config.hpp"
#include "esp_check.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "include/fsm.hpp"
#include "include/io.hpp"
#include "iot_servo.h"

static const char *TAG = "FSM:Humidify";

// Sensor is active low
static constexpr const bool WATER_LEVEL_HIGH = 0;

void SystemFSM::waterLevelSafetyTimerCallback(void *args) {
  auto fsm = static_cast<SystemFSM *>(args);
  ESP_LOGW(TAG, "Water inlet valve safety timer expired, closing valve.");

  gpio_set_level(WATER_INLET_VALVE_PIN, 0);
  fsm->_valveOpen = false;
  xEventGroupSetBits(fsm->_faultFlags, SystemFaults::WaterSensorFailure);
}

void SystemFSM::drainValveCloseTimerCallback(void *args) {
  ESP_LOGI(TAG, "Closing drain valve after dehumidification.");
  gpio_set_level(WATER_DRAIN_VALVE_PIN, 0);
}

esp_err_t SystemFSM::handleHumidifyInit() {
  // create safety timer handle
  const esp_timer_create_args_t timerArgs = {
      .callback = SystemFSM::waterLevelSafetyTimerCallback,
      .arg = this,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "Water Level Safety Timer",
      .skip_unhandled_events = false,
  };

  ESP_RETURN_ON_ERROR(esp_timer_create(&timerArgs, &this->_safetyTimer), TAG,
                      "Failed to create water level safety timer");

  // start timer for 1 minute to close drain valve
  const esp_timer_create_args_t drainTimerArgs = {
      .callback = SystemFSM::drainValveCloseTimerCallback,
      .arg = nullptr,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "Drain Valve Close Timer",
      .skip_unhandled_events = false,
  };

  ESP_RETURN_ON_ERROR(
      esp_timer_create(&drainTimerArgs, &this->_drainCloseTimer), TAG,
      "Failed to create drain valve close timer");

  ESP_RETURN_ON_ERROR(gpio_set_level(WATER_DRAIN_VALVE_PIN, 0), TAG,
                      "Failed to close water inlet valve");

  return ESP_OK;
};

esp_err_t SystemFSM::handleHumidifyPrep() {
  // cancel close drain valve timer if running
  if (this->_drainCloseTimer)
    esp_timer_stop(this->_drainCloseTimer);

  // Disable the fans while we update servos
  intake_fan->setEnabled(false);
  exhaust_fan->setEnabled(false);
  dehumidifier_fan->setEnabled(false);

  // Sets servos to dehumidifier position
  ESP_RETURN_ON_ERROR(iot_servo_write_angle(SERVO_MODE, INTAKE_SERVO_CHANNEL,
                                            INTAKE_SERVO_ANGLE_HUMIDIFIER),
                      TAG, "Failed to set intake servo angle");
  ESP_RETURN_ON_ERROR(iot_servo_write_angle(SERVO_MODE, EXHAUST_SERVO_CHANNEL,
                                            EXHAUST_SERVO_ANGLE_HUMIDIFIER),
                      TAG, "Failed to set exhaust servo angle");

  // close drain valve
  ESP_RETURN_ON_ERROR(gpio_set_level(WATER_DRAIN_VALVE_PIN, 0), TAG,
                      "Failed to close water drain valve");

  // Wait for servos to move
  vTaskDelay(pdMS_TO_TICKS(500));

  this->_currentState = SystemState::HumidifyRun;
  return ESP_OK;
}

esp_err_t SystemFSM::handleHumidifyRun() {
  // Enable all fans
  int fan_speed = (int)(this->_processVars.currentError * HUMIDIFY_FAN_KD);

  // enable atomizer
  ESP_RETURN_ON_ERROR(gpio_set_level(WATER_ATOMIZER_PIN, 1), TAG,
                      "Failed to turn on atomizer");

  intake_fan->setSpeed(fan_speed);
  exhaust_fan->setSpeed(fan_speed);

  intake_fan->setEnabled(true);
  exhaust_fan->setEnabled(true);
  dehumidifier_fan->setEnabled(false);

  // check water level
  int waterLevel = gpio_get_level(WATER_LEVEL_SENSOR_PIN);
  // ESP_LOGI(TAG, "[raw] Water Level Sensor: %d\n", waterLevel);

  // if change, record time (for debounce)
  if (waterLevel != this->_lastWaterLevel) {
    this->_lastWaterLevelChangeTime = esp_timer_get_time();
  }

  uint64_t currentTime = esp_timer_get_time();
  this->_lastWaterLevel = waterLevel;

  if ((currentTime - this->_lastWaterLevelChangeTime) >
      WATER_LEVEL_DEBOUNCE_US) {
    // stable state reached
    ESP_LOGI(TAG, "Stable Water Level: %d", waterLevel);

    if (this->_valveOpen && waterLevel == WATER_LEVEL_HIGH) {
      if (this->_safetyTimer)
        esp_timer_stop(this->_safetyTimer);

      // close the valve
      gpio_set_level(WATER_INLET_VALVE_PIN, 0);
      this->_valveOpen = false;
    } else if (!this->_valveOpen && waterLevel != WATER_LEVEL_HIGH) {
      // open valve
      gpio_set_level(WATER_INLET_VALVE_PIN, 1);
      this->_valveOpen = true;

      // start safety timer
      esp_timer_start_once(this->_safetyTimer,
                           (uint64_t)WATER_MAX_FILL_TIME_SEC * 1'000'000);
    }
  }

  return ESP_OK;
}

esp_err_t SystemFSM::handleHumidifyCleanup() {
  // open drain valve, close inlet valve
  ESP_RETURN_ON_ERROR(gpio_set_level(WATER_INLET_VALVE_PIN, 0), TAG,
                      "Failed to close water inlet valve");
  ESP_RETURN_ON_ERROR(gpio_set_level(WATER_DRAIN_VALVE_PIN, 1), TAG,
                      "Failed to open water drain valve");

  esp_timer_start_once(
      this->_drainCloseTimer,
      static_cast<uint64_t>(60 * 1'000'000)); // close after 1 minute

  return ESP_OK;
}
