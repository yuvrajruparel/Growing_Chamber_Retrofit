#pragma once

#include "fan.hpp"
#include <memory>

/// Fan exhausting air into the chamber
extern std::shared_ptr<PWMFan> intake_fan;

/// Fan below the dessicants
extern std::shared_ptr<PWMFan> dehumidifier_fan;

/// Fan exhausting air to the chamber
extern std::shared_ptr<PWMFan> exhaust_fan;
