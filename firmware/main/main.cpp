#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>

#include "cJSON.h"
#include "config.hpp"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "fan.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "hal/gpio_types.h"
#include "hal/ledc_types.h"
#include "include/fsm.hpp"
#include "include/humidity.hpp"
#include "include/io.hpp"
#include "include/wifi.hpp"
#include "iot_servo.h"

esp_err_t pwm_timer_init();
esp_err_t servo_init();
esp_err_t gpio_init();
esp_err_t fan_init();

std::shared_ptr<PWMFan> intake_fan = nullptr;
std::shared_ptr<PWMFan> dehumidifier_fan = nullptr;
std::shared_ptr<PWMFan> exhaust_fan = nullptr;

static const char *TAG = "main";

// serial mode
#ifdef APP_USE_SERIAL
volatile float g_actual = 0;
volatile float g_setpoint = 0;
std::mutex g_data_mutex;

void serial_reader_task(void *args);
#endif

/**
 * Main application entry point
 */
extern "C" void app_main(void) {
  ESP_ERROR_CHECK(pwm_timer_init());
  ESP_ERROR_CHECK(servo_init());
  ESP_ERROR_CHECK(fan_init());
  ESP_ERROR_CHECK(gpio_init());

  humidity::adcs_init();

#ifdef APP_USE_SERIAL
  // serial mode task
  xTaskCreate(serial_reader_task, "serial_reader", 4096, nullptr, 5, nullptr);
#endif

  // initalize the FSM
  // xTaskCreate(fsm_task, "FSM Task", 8 * 1024, nullptr, 5, nullptr);
  auto fsm = &SystemFSM::getInstance();

  float difference_mv, real_ua = 0;
  FSMProcessVariables vars;

  int i = 0;

  while (true) {
    esp_err_t err0 = humidity::read_real_ua(&real_ua, nullptr);
    esp_err_t err1 = humidity::read_difference_mv(&difference_mv, nullptr);

    if (err0 != ESP_OK || err1 != ESP_OK) {
      ESP_LOGE(TAG,
               "Failed to read ADCs! humidity::read_real_ua ret=%d (%s); "
               "humidity::read_difference_mv ret=%d (%s)",
               err0, strerror(err0), err1, strerror(err1));

      fsm->fault(SystemFaults::AdcReadFailure);
      continue;
    }

#ifdef APP_USE_SERIAL
    {
      std::lock_guard<std::mutex> lock(g_data_mutex);
      vars.currentHumidity = g_actual;
      vars.currentError = g_setpoint - g_actual;
    }
#else
    vars.currentHumidity = (real_ua - 4000.0f) / 16000.0f * 100;
    vars.currentError = difference_mv / 10000.0f * 100 - 50;

    if (i == 10) {
      ESP_LOGI(TAG, "Raw readings -> real_ua: %.2f uA, difference_mv: %.2f mV",
               real_ua, difference_mv);
    }
#endif

    if (i == 10) {
      ESP_LOGI(TAG, "Current humidity: %.2f, error: %.2f", vars.currentHumidity,
               vars.currentError);
      ESP_LOGI(TAG, "FSM State: %s", SystemFSM::stateToString(fsm->getState()));

      i = 0;
    }

    i++;

    fsm->updateProcessVariables(vars);
    fsm->tick();

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

/**
 * Initialize the PWM timer. The PWMFan will init the channels, but not the
 * timer.
 */
esp_err_t pwm_timer_init() {
  ledc_timer_config_t timer_conf = {.speed_mode = FAN_PWM_MODE,
                                    .duty_resolution = FAN_PWM_RESOLUTION,
                                    .timer_num = FAN_PWM_TIMER,
                                    .freq_hz = FAN_PWM_FREQ_HZ,
                                    .clk_cfg = LEDC_AUTO_CLK};
  return ledc_timer_config(&timer_conf);
}

/**
 * Initialize the servo driver
 */
esp_err_t servo_init() {
  // https://www.amazon.com/Smraza-Geared-Control-Arduino-Project/dp/B09Y55C21K
  servo_config_t servo_cfg = {
      .max_angle = 180,
      .min_width_us = 500,
      .max_width_us = 2500,
      .freq = 50,
      .timer_number = SERVO_TIMER,
      .channels = {.servo_pin = {INTAKE_SERVO_PIN, EXHAUST_SERVO_PIN},
                   .ch = {INTAKE_SERVO_CHANNEL, EXHAUST_SERVO_CHANNEL}},
      .channel_number = 2};

  return iot_servo_init(LEDC_LOW_SPEED_MODE, &servo_cfg);
}

/**
 * Initialize all the GPIO pins
 */
esp_err_t gpio_init() {
  gpio_config_t io_conf = {};

  // OUTPUTS
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = (1ULL << WATER_ATOMIZER_PIN) |
                         (1ULL << WATER_INLET_VALVE_PIN) |
                         (1ULL << WATER_DRAIN_VALVE_PIN);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

  gpio_set_level(WATER_ATOMIZER_PIN, 0);
  gpio_set_level(WATER_INLET_VALVE_PIN, 0);
  gpio_set_level(WATER_DRAIN_VALVE_PIN, 0);

  ESP_ERROR_CHECK(gpio_config(&io_conf));

  // INPUTS
  io_conf.intr_type = GPIO_INTR_POSEDGE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1ULL << WATER_LEVEL_SENSOR_PIN);

  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

  ESP_ERROR_CHECK(gpio_config(&io_conf));

  return ESP_OK;
}

/**
 * Initialize all fans
 */
esp_err_t fan_init() {
  gpio_install_isr_service(ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_SHARED);
  PWMFan::wdgStart();

  intake_fan = std::make_shared<PWMFan>(
      PWMFanConfig{.pwm_pin = INTAKE_FAN_PIN,
                   .pwm_channel = INTAKE_FAN_CHANNEL,
                   .pwm_timer = FAN_PWM_TIMER,
                   .pwm_resolution = FAN_PWM_RESOLUTION,
                   .pwm_mode = FAN_PWM_MODE,

                   .enable_pin = INTAKE_FAN_EN_PIN,
                   .tach_pin = INTAKE_FAN_TACH_PIN});

  ESP_ERROR_CHECK(intake_fan->init());
  ESP_ERROR_CHECK(PWMFan::wdgRegisterFan(intake_fan, INTAKE_FAN_FAULT_PIN));

  dehumidifier_fan = std::make_shared<PWMFan>(
      PWMFanConfig{.pwm_pin = DEHUMIDIFIER_FAN_PIN,
                   .pwm_channel = DEHUMIDIFIER_FAN_CHANNEL,
                   .pwm_timer = FAN_PWM_TIMER,
                   .pwm_resolution = FAN_PWM_RESOLUTION,
                   .pwm_mode = FAN_PWM_MODE,

                   .enable_pin = DEHUMIDIFIER_FAN_EN_PIN,
                   .tach_pin = DEHUMIDIFIER_FAN_TACH_PIN});
  ESP_ERROR_CHECK(dehumidifier_fan->init());
  ESP_ERROR_CHECK(
      PWMFan::wdgRegisterFan(dehumidifier_fan, DEHUMIDIFIER_FAN_FAULT_PIN));

  exhaust_fan = std::make_shared<PWMFan>(
      PWMFanConfig{.pwm_pin = EXHAUST_FAN_PIN,
                   .pwm_channel = EXHAUST_FAN_CHANNEL,
                   .pwm_timer = FAN_PWM_TIMER,
                   .pwm_resolution = FAN_PWM_RESOLUTION,
                   .pwm_mode = FAN_PWM_MODE,
                   .enable_pin = EXHAUST_FAN_EN_PIN,
                   .tach_pin = EXHAUST_FAN_TACH_PIN});
  ESP_ERROR_CHECK(exhaust_fan->init());
  ESP_ERROR_CHECK(PWMFan::wdgRegisterFan(exhaust_fan, EXHAUST_FAN_FAULT_PIN));

  return ESP_OK;
}

#ifdef APP_USE_SERIAL
void serial_reader_task(void *arg) {
  const uart_port_t uart_num = UART_NUM_0; // usb UART
  const int rx_buffer_size = 1024;

  if (!uart_is_driver_installed(uart_num)) {
    // We leave TX buffer as 0 so the system log (stdout) bypasses the driver
    // buffer and goes straight to the hardware (preventing conflicts with
    // ESP_LOG).
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(
        uart_driver_install(uart_num, rx_buffer_size, 0, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
  }

  // Allocate temp buffer for raw reads
  auto data = (uint8_t *)malloc(rx_buffer_size);
  std::string line_buffer;

  while (true) {
    // Read data from the UART (non-blocking feel with 20ms timeout)
    // This grabs raw bytes from the RX FIFO/Ring Buffer
    int len = uart_read_bytes(uart_num, data, rx_buffer_size,
                              20 / portTICK_PERIOD_MS);

    if (len > 0) {
      for (int i = 0; i < len; i++) {
        char c = (char)data[i];

        // Handle standard terminals (\n) and raw serial (\r)
        if (c == '\n' || c == '\r') {
          if (!line_buffer.empty()) {
            if (line_buffer.find('{') != std::string::npos) {
              cJSON *root = cJSON_Parse(line_buffer.c_str());
              if (root != nullptr) {
                cJSON *actual_item = cJSON_GetObjectItem(root, "actual");
                cJSON *setpoint_item = cJSON_GetObjectItem(root, "setpoint");

                {
                  std::lock_guard<std::mutex> lock(g_data_mutex);
                  if (cJSON_IsNumber(actual_item))
                    g_actual = (float)actual_item->valuedouble;
                  if (cJSON_IsNumber(setpoint_item))
                    g_setpoint = (float)setpoint_item->valuedouble;

                  ESP_LOGI(TAG, "Updated -> Actual: %.2f, Setpoint: %.2f",
                           g_actual, g_setpoint);
                }
                cJSON_Delete(root);
              } else {
                ESP_LOGW(TAG, "Failed to parse: %s", line_buffer.c_str());
              }
            }
            line_buffer.clear();
          }
        } else {
          line_buffer += c;
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
  free(data);
}
#endif
