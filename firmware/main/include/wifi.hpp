#pragma once
#include "esp_err.h"

namespace wifi {

/**
 * Connects to the Duke wifi.
 */
esp_err_t connect();

/**
 * Posts all the sensor data
 */
void post_sensor_data(float real_ua, int real_raw, float sp_mv, int sp_raw);

/**
 * Returns if the wifi is ready
 */
bool ready();

} // namespace wifi
