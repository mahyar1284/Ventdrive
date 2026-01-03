#include "MotionVisor.hpp"
#include <functional>

MotionVisor::MotionVisor(): timer(TIM3)
{
    pinMode(endstopPin, INPUT_PULLDOWN);
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);

    timer.setOverflow(100, MICROSEC_FORMAT);
    timer.attachInterrupt(std::bind(&MotionVisor::stepperAsyncLoop, this));
    timer.resume();
}

void MotionVisor::stepperAsyncLoop()
{
    if(autoHomeFlag)
    {
        static int cnt;
        unsigned long delay = (unsigned long)(10000.0 / (config.speed * config.stepPermm));
        // signed long softStart = 500 - std::abs(goalStep);
        digitalWrite(dirPin, config.invertDir ? LOW : HIGH);
        if(cnt++ > delay)
        {
            if(goalStep < 0 and !isAtEndstop())
            {
                goalStep ++;
                digitalToggle(stepPin); // rotate 1 step
            }
            else 
            {
                _state = MotionVisorState::Idle;
                currentStep = 0;
                goalStep = 0;
                autoHomeFlag = false;
            }
            cnt = 0;
        }
    }
    else if(currentStep.has_value())
    {
        static int cnt = 0;

        // base delay in ticks
        unsigned long delay = (unsigned long)(10000.0 / (config.speed * config.stepPermm));
        //auto delay = computeDelayTicks(goalStep, currentStep.value(), totalDistSteps, config.speed, config.stepPermm, config.acceleration);

        if(cnt++ > delay) {
            if(currentStep.value() < goalStep) {
                digitalWrite(dirPin, config.invertDir ? HIGH : LOW);
                digitalToggle(stepPin);
                currentStep = currentStep.value() + 1;
                _state = MotionVisorState::Opening;
            } else if(currentStep.value() > goalStep) {
                digitalWrite(dirPin, config.invertDir ? LOW : HIGH);
                digitalToggle(stepPin);
                currentStep = currentStep.value() - 1;
                _state = MotionVisorState::Closing;
            } else if(_state != MotionVisorState::Error) {
                _state = MotionVisorState::Idle;
            }
            cnt = 0;
        }
    }
}

MotionVisor::~MotionVisor()
{

}

void MotionVisor::loop()
{
    if(_state == MotionVisorState::Idle)// if not moving
    {
        if(isAtEndstop()) // check if the vent state
        {
            if(currentStep.has_value())
            {
                if(currentStep > mmToStep(config.endstopMinDistance))
                    _state = MotionVisorState::Error; // it should be opened but endstop is sensing a closed state
            }
            else
            {
                currentStep = 0; // closed state
                _state = MotionVisorState::Idle;
            }
        }
        else
        {
            if(currentStep.has_value())
            {
                if(currentStep == 0)
                    _state = MotionVisorState::Error; // it should be closed but endstop ain't sensing a closed state
            }
        }
    }
    if(_state == MotionVisorState::Closing and isAtEndstop()) // to avoid mechanical damage if reached endstop sooner than expected
    {
        currentStep = 0;
    }
}

unsigned long MotionVisor::mmToStep(double mm)
{
    return (unsigned long)(mm * config.stepPermm);
}

void MotionVisor::setVentingPercent(int percent)
{
    if(currentStep.has_value()) // if autoHomed
    {
        if(percent > 100) percent = 100; // upper limit
        if(percent < 10) percent = 0; // lower limit
        auto calculatedSteps = mmToStep(config.length * ((double)percent / 100.0));
        if(goalStep != calculatedSteps) // if data is new
        {
            goalStep = calculatedSteps;
            totalDistSteps = std::abs(currentStep.value() - goalStep);
        }
    }
}

std::optional<int> MotionVisor::ventingPercent()
{
    if(currentStep.has_value())
    {
        return ((double)currentStep.value() / mmToStep(config.length)) * 100;
    }
    return std::nullopt;
}


void MotionVisor::autoHome()
{
    if(!autoHomeFlag)
    {        
        autoHomeFlag = true;
        _state = MotionVisorState::Closing;
        goalStep = -mmToStep(config.length + config.maxCompensation);
    }
}

void MotionVisor::setConfig(const MotionVisorConfig &config)
{
    this->config = config;
}

bool MotionVisor::isAtEndstop()
{
    return config.invertEndstopPin ? !digitalRead(endstopPin) : digitalRead(endstopPin);
}