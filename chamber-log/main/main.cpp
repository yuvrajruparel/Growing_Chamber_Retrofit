#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "include/humidity.hpp"
#include "include/wifi.hpp"

const char *TAG = "main";

extern "C" void app_main(void) {
  humidity::adcs_init();
  wifi::connect();

  // wait for wifi establish
  ESP_LOGI(TAG, "Waiting for wifi...");
  while (!wifi::ready()) {
    printf(".");
    fflush(stdout);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  printf("\n");

  while (true) {
    int real_raw, sp_raw;
    float real_ua, sp_mv;

    ESP_ERROR_CHECK_WITHOUT_ABORT(humidity::read_real_ua(&real_ua, &real_raw));
    ESP_ERROR_CHECK_WITHOUT_ABORT(humidity::read_setpoint_mv(&sp_mv, &sp_raw));

    ESP_LOGI(TAG, "Real humidity: %f uA (raw=%f)", real_ua, real_raw);
    ESP_LOGI(TAG, "Setpoint humidity: %f mV (raw=%f)", sp_mv, sp_raw);

    wifi::post_sensor_data(real_ua, real_raw, sp_mv, sp_raw);

    // in prod, probably best to enter light/deep sleep to save power
    // but for testing, this is ok so we don't have to worry about added
    // complexity of deinit-ing the wifi driver, etc.
    vTaskDelay(pdMS_TO_TICKS(5 * 60'000));
  }
}
