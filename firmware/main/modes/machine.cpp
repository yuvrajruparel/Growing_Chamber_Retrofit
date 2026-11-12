#include "config.hpp"
#include "esp_log.h"
#include "include/fsm.hpp"

static const char *TAG = "SystemFSM";

void SystemFSM::updateProcessVariables(const FSMProcessVariables &vars) {
  std::lock_guard<std::mutex> lock(this->_mutex);
  this->_processVars = vars;
}

void SystemFSM::tick() {
  std::lock_guard<std::mutex> lock(this->_mutex);

  SystemState nextState = this->_currentState;

  // look for state transitions only if in RUN states
  if ((this->_currentState == SystemState::HumidifyRun ||
       this->_currentState == SystemState::Idle) &&
      this->_processVars.currentError < -HUMIDITY_DEADBAND) {
    nextState = SystemState::DehumidifyPrep;
  } else if ((this->_currentState == SystemState::DehumidifyRun ||
              this->_currentState == SystemState::Idle) &&
             this->_processVars.currentError > HUMIDITY_DEADBAND) {
    nextState = SystemState::HumidifyPrep;
  } else if ((this->_currentState == SystemState::HumidifyRun ||
              this->_currentState == SystemState::DehumidifyRun) &&
             std::abs(this->_processVars.currentError) <= HUMIDITY_DEADBAND) {
    nextState = SystemState::Idle;
  }

  // if state changed, log it
  if (nextState != this->_currentState) {
    ESP_LOGI(TAG, "State changed from %s to %s",
             stateToString(this->_currentState), stateToString(nextState));

    // if transitioning away from humidify, run cleanup
    if (this->_currentState == SystemState::HumidifyRun) {
      ESP_LOGI(TAG, "Running humidify cleanup");
      ESP_ERROR_CHECK(this->handleHumidifyCleanup());
    }

    this->_currentState = nextState;
  }

  // run the respective state logic
  esp_err_t err = ESP_OK;

  switch (this->_currentState) {
  case SystemState::Idle:
    err = this->handleIdle();
    break;

  case SystemState::HumidifyPrep:
    err = this->handleHumidifyPrep();
    break;

  case SystemState::HumidifyRun:
    err = this->handleHumidifyRun();
    break;

  case SystemState::DehumidifyPrep:
    err = this->handleDehumidifyPrep();
    break;

  case SystemState::DehumidifyRun:
    err = this->handleDehumidifyRun();
    break;

  case SystemState::Fault:
    ESP_LOGE(TAG, "System is in fault state.");
    this->_currentState = SystemState::Idle;
    break;
  }

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error in state handler for state %s: %s",
             stateToString(this->_currentState), esp_err_to_name(err));
    this->_currentState = SystemState::Fault;
  }
}

SystemState SystemFSM::getState() {
  std::lock_guard<std::mutex> lock(this->_mutex);
  return this->_currentState;
}

SystemFSM::SystemFSM() {
  ESP_LOGI(TAG, "Initializing System FSM");

  this->_faultFlags = xEventGroupCreate();
  ESP_ERROR_CHECK(this->handleHumidifyInit());
}

void SystemFSM::fault(SystemFaults::Type flags) {
  std::lock_guard<std::mutex> lock(this->_mutex);
  xEventGroupSetBits(this->_faultFlags, SystemFaults::WaterSensorFailure);
  this->_currentState = SystemState::Fault;
}
