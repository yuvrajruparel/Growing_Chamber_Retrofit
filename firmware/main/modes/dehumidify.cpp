#include "config.hpp"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "include/fsm.hpp"
#include "include/io.hpp"
#include "iot_servo.h"

static const char *TAG = "FSM:Dehumidify";

esp_err_t SystemFSM::handleDehumidifyPrep() {
  // Disable the fans while we update servos
  intake_fan->setEnabled(false);
  exhaust_fan->setEnabled(false);
  dehumidifier_fan->setEnabled(false);

  ESP_RETURN_ON_ERROR(gpio_set_level(WATER_ATOMIZER_PIN, 0), TAG,
                      "Failed to turn off atomizer");

  // Sets servos to dehumidifier position
  ESP_RETURN_ON_ERROR(iot_servo_write_angle(SERVO_MODE, INTAKE_SERVO_CHANNEL,
                                            INTAKE_SERVO_ANGLE_DEHUMIDIFIER),
                      TAG, "Failed to set intake servo angle");
  ESP_RETURN_ON_ERROR(iot_servo_write_angle(SERVO_MODE, EXHAUST_SERVO_CHANNEL,
                                            EXHAUST_SERVO_ANGLE_DEHUMIDIFIER),
                      TAG, "Failed to set exhaust servo angle");

  // Wait for servos to move
  vTaskDelay(pdMS_TO_TICKS(500));

  this->_currentState = SystemState::DehumidifyRun;
  return ESP_OK;
}

esp_err_t SystemFSM::handleDehumidifyRun() {
  // Enable all fans
  int fan_speed =
      (int)(std::abs(this->_processVars.currentError) * DEHUMIDIFY_FAN_KD);

  intake_fan->setSpeed(fan_speed);
  exhaust_fan->setSpeed(fan_speed);
  dehumidifier_fan->setSpeed(fan_speed);

  intake_fan->setEnabled(true);
  exhaust_fan->setEnabled(true);
  dehumidifier_fan->setEnabled(true);

  // drain valve open during dehumid, inlet valve closed
  ESP_RETURN_ON_ERROR(gpio_set_level(WATER_INLET_VALVE_PIN, 0), TAG,
                      "Failed to close water inlet valve");

  return ESP_OK;
}
