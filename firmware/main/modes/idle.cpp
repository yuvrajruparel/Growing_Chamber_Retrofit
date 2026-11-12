#include "config.hpp"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "include/fsm.hpp"
#include "include/io.hpp"
#include "iot_servo.h"

static const char *TAG = "SystemFSM";

esp_err_t SystemFSM::handleIdle() {
  // Fans should be disabled and servo set to HUMIDIFY position
  intake_fan->setEnabled(false);
  exhaust_fan->setEnabled(false);
  dehumidifier_fan->setEnabled(false);
  gpio_set_level(WATER_ATOMIZER_PIN, 0);

  ESP_RETURN_ON_ERROR(iot_servo_write_angle(SERVO_MODE, INTAKE_SERVO_CHANNEL,
                                            INTAKE_SERVO_ANGLE_HUMIDIFIER),
                      TAG, "Failed to set intake servo angle in Idle");

  ESP_RETURN_ON_ERROR(iot_servo_write_angle(SERVO_MODE, EXHAUST_SERVO_CHANNEL,
                                            EXHAUST_SERVO_ANGLE_HUMIDIFIER),
                      TAG, "Failed to set exhaust servo angle in Idle");

  // disable inlet valves
  ESP_RETURN_ON_ERROR(gpio_set_level(WATER_INLET_VALVE_PIN, 0), TAG,
                      "Failed to close water inlet valve in Idle");

  return ESP_OK;
}
