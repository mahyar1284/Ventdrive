#pragma once

struct MotionVisorConfig
{
    bool invertDir = false; // based on motor connection
    bool invertEndstopPin = false; // based on sensor type
    double stepPermm = 0.005; // step count per each mm of movement
    double speed = 8; // mm/s
    double length = 30; // mm (vent length)
    double maxCompensation = 5; // mm (affects only closing)
};
