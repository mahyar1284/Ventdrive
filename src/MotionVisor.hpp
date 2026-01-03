#pragma once
#include "MotionVisorState.hpp"
#include "MotionVisorConfig.hpp"
#include <arduino.h>
#include <HardwareTimer.h>

class MotionVisor
{
public:
    MotionVisor();
    ~MotionVisor();

    void setConfig(const MotionVisorConfig &config);
    MotionVisorConfig getConfig() {return config;}
    void setVentingPercent(int percent);
    std::optional<int> ventingPercent();
    void autoHome();
    void loop();
    MotionVisorState state() const { return _state; }


private:
    void stepperAsyncLoop();
    unsigned long mmToStep(double mm);
    bool isAtEndstop();

    HardwareTimer timer;
    MotionVisorState _state = MotionVisorState::Uninitialized;
    int dirPin = PC14, stepPin = PC15, enPin = PB4, endstopPin = PA8;
    MotionVisorConfig config;
    long goalStep = 0;
    long totalDistSteps = 0;
    std::optional<long> currentStep = std::nullopt;
    double vMax = 0;
    double aSteps = 0;
    bool autoHomeFlag = false;

// unsigned long computeDelayTicks(
//     int goalStep,
//     int currentStep,
//     int totalDistSteps,
//     double speed_mm_s,
//     double steps_per_mm,
//     double acc_mm_s2
// ) {
//     int diff    = goalStep - currentStep;
//     int absDiff = abs(diff);
//     int stepsTravelled = totalDistSteps - absDiff;
//     if (stepsTravelled < 0) stepsTravelled = 0;

//     double vMax   = speed_mm_s * steps_per_mm;   // steps/s
//     double aSteps = acc_mm_s2 * steps_per_mm;    // steps/s^2

//     if (vMax <= 0.0 || aSteps <= 0.0) return 0; // safe large delay

//     // acceleration cap
//     double vAcc = sqrt(2.0 * aSteps * (double)stepsTravelled);
//     if (vAcc < 1.0) vAcc = 1.0;

//     // deceleration cap
//     double vDec = sqrt(2.0 * aSteps * (double)absDiff);
//     if (vDec < 1.0) vDec = 1.0;

//     // commanded velocity
//     double vCmd = std::min(vMax, std::min(vAcc, vDec));

//     // convert to ISR ticks (100us = 1 tick)
//     double delayTicks = 10000.0 / vCmd;
//     if (delayTicks < 1.0) delayTicks = 1.0;
//     if (delayTicks > 1000000.0) delayTicks = 1000000.0;

//     return (unsigned long)delayTicks;
// }
};