#pragma once
#include "MotionVisorState.hpp"
#include "MotionVisorConfig.hpp"
#include <arduino.h>
#include <HardwareTimer.h>

class MotionVisor
{
public:
    enum class Direction
    {
        Forward,
        Backward
    };
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
    void disableStepper();
    void enableStepper();
    void moveOneStep(Direction direction);

    HardwareTimer timer;
    MotionVisorState _state = MotionVisorState::Uninitialized;
    int dirPin = PC14, stepPin = PC15, enPin = PB0, endstopPin = PB1;
    MotionVisorConfig config;
    long goalStep = 0;
    long totalDistSteps = 0;
    std::optional<long> currentStep = std::nullopt;
    double vMax = 0;
    double aSteps = 0;
    bool autoHomeFlag = false;
};