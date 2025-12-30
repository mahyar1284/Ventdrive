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
    void open();
    void close();
    void autoHome();
    void loop();
    MotionVisorState state() const { return _state; }


private:
    void stepperAsyncLoop();
    unsigned long mmToStep(double mm);
    bool isAtEndstop();

    HardwareTimer timer;
    MotionVisorState _state = MotionVisorState::Error;
    int dirPin = PC14, stepPin = PC15, enPin = PB4, endstopPin = PB5;
    MotionVisorConfig config;
    long currentStep = 0, goalStep = 0;
};