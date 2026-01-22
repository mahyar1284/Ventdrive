#pragma once
#include <optional>

struct MotionVisorConfig
{
    bool invertDir = true; // based on motor connection
    bool invertEndstopPin = true; // based on sensor type
    bool isEndstopAtClosedState = true; // based on endstop placement (Not Implemented Yet!!!!)
    double endstopExtraDistance = 5; // extra distance to take after sensing endstop
    double stepPermm = 200.0; // step count per each mm of movement
    double speed = 8; // mm/s
    double length = 30; // mm (vent length)
    double maxCompensation = 5; // mm (affects only closing)
    double acceleration = 20; // mm/s2
};
