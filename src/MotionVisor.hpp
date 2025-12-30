#pragma once
#include "MotionVisorState.hpp"
#include <arduino.h>
#include <HardwareTimer.h>

class MotionVisor
{
public:
    MotionVisor();
    ~MotionVisor();

    void init();
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
    bool invertDir = false, invertEndstopPin = false;
    double stepPermm = 0.005, speed = 8, length = 30, maxCompensation = 5;
    long currentStep = 0, goalStep = 0;
};