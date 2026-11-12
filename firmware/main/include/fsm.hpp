#pragma once

#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/idf_additions.h"
#include <atomic>
#include <cstdint>
#include <mutex>

namespace SystemFaults {

using Type = std::uint32_t;

/* clang-format off */
static constexpr Type None                   = 0;
static constexpr Type Generic                = (1 << 0); // 0x01
static constexpr Type FanIntakeFailure       = (1 << 1); // 0x02
static constexpr Type FanDehumidifierFailure = (1 << 2); // 0x04
static constexpr Type FanExhaustFailure      = (1 << 3); // 0x08
static constexpr Type WaterSensorFailure     = (1 << 4); // 0x10
static constexpr Type WaterInValveFailure    = (1 << 5); // 0x20
static constexpr Type WaterDrainValveFailure = (1 << 6);
static constexpr Type AdcReadFailure         = (1 << 7);
/* clang-format on */

/**
 * Check if a fault flag is set
 */
inline bool hasFault(Type flags, Type check) { return (flags & check) != 0; }

} // namespace SystemFaults

/**
 * The states for the system's FSM
 */
enum SystemState : std::uint8_t {
  /**
   * Humidity is at setpoint, system is idle
   * Valid transitions: -> HUMIDIFY_PREP, DEHUMIDIFY_PREP
   */
  Idle,

  /**
   * Preparing to humidify the environment (moving servos, etc)
   * Valid transitions: -> HUMIDIFY_RUN, IDLE
   */
  HumidifyPrep,

  /**
   * Actively humidifying the environment
   * Valid transitions: -> IDLE
   */
  HumidifyRun,

  /**
   * Preparing to dehumidify the environment (moving servos, etc)
   * Valid transitions: -> DEHUMIDIFY_RUN, IDLE
   */
  DehumidifyPrep,

  /**
   * Actively dehumidifying the environment
   * Valid transitions: -> IDLE
   */
  DehumidifyRun,

  /**
   * System is in a fault state
   * Valid transitions: -> IDLE (after fault is cleared)
   */
  Fault
};

/**
 * Holds the process variables
 */
struct FSMProcessVariables {
  float currentHumidity = 0.0f;
  float currentError = 0.0f;
};

/**
 * Singleton FSM to manage the system state
 */
class SystemFSM {
public:
  static const char *stateToString(SystemState state) {
    switch (state) {
    case SystemState::Idle:
      return "Idle";
    case SystemState::HumidifyPrep:
      return "HumidifyPrep";
    case SystemState::HumidifyRun:
      return "HumidifyRun";
    case SystemState::DehumidifyPrep:
      return "DehumidifyPrep";
    case SystemState::DehumidifyRun:
      return "DehumidifyRun";
    case SystemState::Fault:
      return "Fault";
    default:
      return "Unknown";
    }
  }

  static SystemFSM &getInstance() {
    static SystemFSM instance;
    return instance;
  }

  // Delete copy constructor to prevent duplicates
  SystemFSM(const SystemFSM &) = delete;
  void operator=(const SystemFSM &) = delete;

  void tick();
  SystemState getState();
  void updateProcessVariables(const FSMProcessVariables &vars);

  /**
   * Moves the FSM to the fault state.
   */
  void fault(SystemFaults::Type flags);

private:
  SystemFSM();

  SystemState _currentState = SystemState::Idle;
  std::mutex _mutex;
  FSMProcessVariables _processVars;

  // bitfield stores system faults
  EventGroupHandle_t _faultFlags;

  // for managing water level
  std::atomic<bool> _valveOpen = false;
  int _lastWaterLevel = -1;
  uint64_t _lastWaterLevelChangeTime = 0;

  esp_timer_handle_t _safetyTimer = nullptr;
  esp_timer_handle_t _drainCloseTimer = nullptr;

  // Call once at initialization
  esp_err_t handleHumidifyInit();

  // Call each of these per tick based on state
  esp_err_t handleHumidifyPrep();
  esp_err_t handleHumidifyRun();
  esp_err_t handleHumidifyCleanup();
  esp_err_t handleDehumidifyPrep();
  esp_err_t handleDehumidifyRun();
  esp_err_t handleIdle();

  // callbacks for ISRs
  static void waterLevelSafetyTimerCallback(void *args);
  static void drainValveCloseTimerCallback(void *args);
};
