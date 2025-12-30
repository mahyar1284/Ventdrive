#include "MotionVisor.hpp"
#include <functional>

MotionVisor::MotionVisor(): timer(TIM3)
{
    pinMode(endstopPin, INPUT_PULLDOWN);
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    if(digitalRead(endstopPin))
        _state = MotionVisorState::Closed;
    else
        _state = MotionVisorState::Opened;

    
    timer.setOverflow(100, MICROSEC_FORMAT);
    timer.attachInterrupt(std::bind(&MotionVisor::stepperAsyncLoop, this));
    timer.resume();
}

void MotionVisor::stepperAsyncLoop()
{
    static int cnt = 0;
    unsigned long delay = (config.stepPermm / config.speed) * 10000;
    signed long softStart = 500 - currentStep;
    signed long softStop = 500 - (goalStep - currentStep);
    if(softStart < 0) softStart = 0;
    if(softStop < 0) softStop = 0;
    if(cnt++ > delay + (softStart / 10) + (softStop / 10))
    {
        if(currentStep++ < goalStep)
        {
            digitalToggle(stepPin); // rotate 1 step
        }
        else
        {
            goalStep = 0;
            currentStep = 0;
        }
        cnt = 0;
    }
}

MotionVisor::~MotionVisor()
{

}

void MotionVisor::loop()
{
    if(goalStep == 0) // if not moving
    {
        if(isAtEndstop()) // check if the vent state
            _state = MotionVisorState::Closed;
        else
            _state = MotionVisorState::Opened;
    }
    if((goalStep - currentStep) > 0)
    {
        if(_state == MotionVisorState::Closing and isAtEndstop()) // to avoid mechanical damage if reached endstop sooner than expected
        {
            goalStep = 0;
            currentStep = 0;
        }
    }
}

unsigned long MotionVisor::mmToStep(double mm)
{
    return (double)(config.length / config.stepPermm);
}

void MotionVisor::open()
{
    digitalWrite(dirPin, config.invertDir ? HIGH : LOW);
    currentStep = 0;
    goalStep = mmToStep(config.length);
    _state = MotionVisorState::Opening;
}

void MotionVisor::close()
{
    digitalWrite(dirPin, config.invertDir ? LOW : HIGH);
    currentStep = 0;
    goalStep = mmToStep(config.length + config.maxCompensation); // compensate for missed steps 
    _state = MotionVisorState::Closing;
}

void MotionVisor::autoHome()
{

}

void MotionVisor::setConfig(const MotionVisorConfig &config)
{
    this->config = config;
}

bool MotionVisor::isAtEndstop()
{
    return config.invertEndstopPin ? !digitalRead(endstopPin) : digitalRead(endstopPin);
}