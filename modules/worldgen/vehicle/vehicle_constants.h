#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "core/math/math_defs.h"

namespace VehicleConstants {
constexpr float AV_TO_RPM = 60.0 / Math_TAU;
constexpr float RPM_TO_AV = 1.0 / AV_TO_RPM;
}; // namespace VehicleConstants

#endif // CONSTANTS_H
