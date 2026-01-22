#include "MotionVisor.hpp"
#include <functional>

MotionVisor::MotionVisor(): timer(TIM3)
{
    pinMode(endstopPin, INPUT_PULLDOWN);
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    pinMode(enPin, OUTPUT);

    timer.setOverflow(100, MICROSEC_FORMAT);
    timer.attachInterrupt(std::bind(&MotionVisor::stepperAsyncLoop, this));
    timer.resume();
}

void MotionVisor::stepperAsyncLoop()
{
    if(autoHomeFlag)
    {
        static int extraStepsCounter = 0;
        static int cnt;
        unsigned long delay = (unsigned long)(10000.0 / (config.speed * config.stepPermm));
        digitalWrite(enPin, LOW);
        digitalWrite(dirPin, config.invertDir ? LOW : HIGH);
        if(cnt++ > delay)
        {
            if(goalStep < 0 and !isAtEndstop())
            {
                goalStep ++;
                digitalWrite(stepPin, HIGH); // rotate 1 step
                digitalWrite(stepPin, LOW); // rotate 1 step
            }
            else 
            {
                if(isAtEndstop())
                {
                    if(extraStepsCounter++ > mmToStep(config.endstopExtraDistance))
                    {
                        extraStepsCounter = 0;
                        _state = MotionVisorState::Idle;
                        currentStep = 0;
                        goalStep = 0;
                        autoHomeFlag = false;
                    }
                    else
                    {
                        digitalWrite(stepPin, HIGH); // rotate an extra step
                        digitalWrite(stepPin, LOW);
                    }
                }
                else
                {
                    _state = MotionVisorState::Error;
                    currentStep = std::nullopt;
                    goalStep = 0;
                    autoHomeFlag = false;
                }
            }
            cnt = 0;
        }
    }
    else if(currentStep.has_value())
    {
        static int extraStepsCounter = 0;
        static int cnt = 0;
        // base delay in ticks
        unsigned long delay = (unsigned long)(10000.0 / (config.speed * config.stepPermm));
        //auto delay = computeDelayTicks(goalStep, currentStep.value(), totalDistSteps, config.speed, config.stepPermm, config.acceleration);

        if(cnt++ > delay) 
        {
            if(currentStep.value() < goalStep) 
            {
                digitalWrite(enPin, LOW);
                digitalWrite(dirPin, config.invertDir ? HIGH : LOW);
                digitalWrite(stepPin, HIGH); // rotate 1 step
                digitalWrite(stepPin, LOW); // rotate 1 step
                currentStep = currentStep.value() + 1;
                _state = MotionVisorState::Opening;
            } else if(currentStep.value() > goalStep and !isAtEndstop()) 
            {
                digitalWrite(enPin, LOW);
                digitalWrite(dirPin, config.invertDir ? LOW : HIGH);
                digitalWrite(stepPin, HIGH); // rotate 1 step
                digitalWrite(stepPin, LOW); // rotate 1 step
                currentStep = currentStep.value() - 1;
                _state = MotionVisorState::Closing;
            } 
            else if(isAtEndstop() and _state == MotionVisorState::Closing)
            {
                if(extraStepsCounter++ > mmToStep(config.endstopExtraDistance))
                {
                    extraStepsCounter = 0;
                    currentStep = 0; // is at home(origin) so currentStep should be zero
                    _state = MotionVisorState::Idle;
                }
                else // rotate an extra step until extraStepsCounter reaches mmToStep(endstopExtraDistance)
                {
                    digitalWrite(dirPin, config.invertDir ? LOW : HIGH);
                    digitalWrite(stepPin, HIGH);
                    digitalWrite(stepPin, LOW);
                }
            }
            else 
            {
                if(_state != MotionVisorState::Error) 
                {
                    _state = MotionVisorState::Idle;
                }
                digitalWrite(enPin, HIGH); // disable stepper
            }
            cnt = 0;
        }
    }
    else
    {
        digitalWrite(enPin, HIGH); // disable stepper
    }
}

MotionVisor::~MotionVisor()
{

}

void MotionVisor::loop()
{
    if(!autoHomeFlag)
    {
        if(_state == MotionVisorState::Idle)// if not moving
        {
            if(isAtEndstop()) // check if the vent state
            {
                if(currentStep.has_value())
                {
                    if(currentStep.value() > mmToStep(config.endstopExtraDistance))
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
                    if(currentStep.value() == 0)
                        _state = MotionVisorState::Error; // it should be closed but endstop ain't sensing a closed state
                }
            }
        }
    }
}

unsigned long MotionVisor::mmToStep(double mm)
{
    return (unsigned long)(mm * config.stepPermm);
}

void MotionVisor::setVentingPercent(int percent)
{
    if(currentStep.has_value() and _state != MotionVisorState::Error) // if autoHomed
    {
        double lowerLimit = (((config.endstopExtraDistance * 2.0) / config.length) * 100.0); // Percent of (2 x endstopExtraDistance)
        if(percent > 100) percent = 100; // upper limit
        if(percent < lowerLimit) percent = 0; // lower limit
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
        if(!isAtEndstop())
        {
            autoHomeFlag = true;
            _state = MotionVisorState::Closing;
            goalStep = -mmToStep(config.length + config.maxCompensation);
        }
        else // already is at origin
        {
            _state = MotionVisorState::Idle;
            currentStep = 0;
            goalStep = 0;
        }
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