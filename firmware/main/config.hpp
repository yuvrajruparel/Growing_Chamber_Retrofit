#pragma once
#include "driver/ledc.h"
#include "hal/adc_types.h"

/*****************************
 *    GENERAL SETTINGS       *
 *****************************/
// Any |error| < DEADBAND is considered acceptable
#define HUMIDITY_DEADBAND 2
#define DEHUMIDIFY_FAN_KD 8.0f
#define HUMIDIFY_FAN_KD 8.0f

/*****************************
 *      WIFI SETTINGS        *
 *****************************/

#define WIFI_NAME "DukeOpen"
#define WIFI_PASSWORD ""

#define HTTP_ENDPOINT                                                          \
  "https://n8n.derock.dev/webhook/8d5e7771-8bd3-4d71-8ac8-5746f56ada5a"

/******************************
 *   TC2 INTERFACE SETTINGS   *
 ******************************/

// Must use ADC1, ADC2 is unstable when wifi is in use.
#define VOLTAGE_ADC_CHANNEL ADC_CHANNEL_7 // GPIO35 for Setpoint (0-10V)
#define CURRENT_ADC_CHANNEL ADC_CHANNEL_6 // GPIO34 for Real (4-20mA)

#define R1_OHMS 22000.0f           // Voltage divider top resistor
#define R2_OHMS 10000.0f           // Voltage divider bottom resistor
#define SHUNT_RESISTOR_OHMS 100.0f // Shunt resistor for current measurement

// Calibration constants for voltage and current measurements
// measure at 4mA and 20mA, then record the mV values (non converted, raw adc)

#define V_LOW_MV 547    // The measured mV (raw adc) at 4mA
#define V_HIGH_MV 2462  // The measured mV (raw adc) at 20mA
#define I_LOW_UA 4000   // 4mA
#define I_HIGH_UA 20000 // 20mA

#define SETPOINT_RAW_HIGH 4090 // Raw ADC reading at 10,000mV
#define SETPOINT_RAW_LOW 597   // Raw ADC reading at 2,000mV
#define SETPOINT_MV_HIGH 10000 // 10,000mV
#define SETPOINT_MV_LOW 2000   // 2,000mV

/*****************************
 *       FAN SETTINGS        *
 *****************************/

#define FAN_PWM_FREQ_HZ 25000
#define FAN_PWM_RESOLUTION LEDC_TIMER_10_BIT

// timer 1 is 10-bit, suitable for fan speed control
#define FAN_PWM_TIMER LEDC_TIMER_1

#define FAN_PWM_MODE LEDC_HIGH_SPEED_MODE
// #define FAN_PWM_MODE LEDC_LOW_SPEED_MODE

/** INTAKE FAN **/
#define INTAKE_FAN_PIN GPIO_NUM_21
#define INTAKE_FAN_EN_PIN GPIO_NUM_23
#define INTAKE_FAN_TACH_PIN GPIO_NUM_22
#define INTAKE_FAN_CHANNEL LEDC_CHANNEL_0
#define INTAKE_FAN_FAULT_PIN GPIO_NUM_NC

// dehumid intake
#define DEHUMIDIFIER_FAN_PIN GPIO_NUM_5
#define DEHUMIDIFIER_FAN_EN_PIN GPIO_NUM_19
#define DEHUMIDIFIER_FAN_TACH_PIN GPIO_NUM_18
#define DEHUMIDIFIER_FAN_CHANNEL LEDC_CHANNEL_1
#define DEHUMIDIFIER_FAN_FAULT_PIN GPIO_NUM_NC

// exhaust fan
#define EXHAUST_FAN_PIN GPIO_NUM_4
#define EXHAUST_FAN_EN_PIN GPIO_NUM_17
#define EXHAUST_FAN_TACH_PIN GPIO_NUM_16
#define EXHAUST_FAN_CHANNEL LEDC_CHANNEL_2
#define EXHAUST_FAN_FAULT_PIN GPIO_NUM_NC

/*****************************
 *      SERVO SETTINGS       *
 *****************************/

// timer 0 provides greater accuracy for servos due to its 14-bit resolution
#define SERVO_TIMER LEDC_TIMER_0
#define SERVO_MODE LEDC_LOW_SPEED_MODE

// M5 on PCB
#define INTAKE_SERVO_PIN GPIO_NUM_14
#define INTAKE_SERVO_CHANNEL LEDC_CHANNEL_7
#define INTAKE_SERVO_ANGLE_HUMIDIFIER 67
#define INTAKE_SERVO_ANGLE_DEHUMIDIFIER 102

// M4 on PCB
#define EXHAUST_SERVO_PIN GPIO_NUM_12
#define EXHAUST_SERVO_CHANNEL LEDC_CHANNEL_6
#define EXHAUST_SERVO_ANGLE_HUMIDIFIER 67
#define EXHAUST_SERVO_ANGLE_DEHUMIDIFIER 115

/***************************
 *   HUMIDIFIER SETTINGS   *
 ***************************/

// digital 1 or 0 for water level sensor
// #define WATER_LEVEL_SENSOR_PIN GPIO_NUM_10
#define WATER_MAX_FILL_TIME_SEC 5
#define WATER_LEVEL_DEBOUNCE_US 500000 // 500ms debounce
// #define WATER_INLET_VALVE_PIN GPIO_NUM_6 // V1
// #define WATER_DRAIN_VALVE_PIN GPIO_NUM_7 // V2
#define WATER_ATOMIZER_PIN GPIO_NUM_32

// MOVED from GPIO 10 (Flash SD3) -> GPIO 27
// JUMP D3 to 27
#define WATER_LEVEL_SENSOR_PIN GPIO_NUM_27

// MOVED from GPIO 6 (Flash SCK) -> GPIO 25
// JUMP CLK to 26
#define WATER_INLET_VALVE_PIN GPIO_NUM_26

// MOVED from GPIO 7 (Flash SDO) -> GPIO 26
#define WATER_DRAIN_VALVE_PIN GPIO_NUM_25

// if valve is open for more than this, something is wrong
#define VALVE_OPEN_TIMEOUT_MS 5000
