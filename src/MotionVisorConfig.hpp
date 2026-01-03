#pragma once
#include <optional>

struct MotionVisorConfig
{
    bool invertDir = false; // based on motor connection
    bool invertEndstopPin = false; // based on sensor type
    double endstopMinDistance = 5; // minimum required distance to release endstop
    double stepPermm = 200.0; // step count per each mm of movement
    double speed = 8; // mm/s
    double length = 30; // mm (vent length)
    double maxCompensation = 5; // mm (affects only closing)
    double acceleration = 20; // mm/s2
};
