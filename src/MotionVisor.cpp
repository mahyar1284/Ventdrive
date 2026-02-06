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
        enableStepper();
        if(cnt++ > delay)
        {
            if(goalStep < 0 and !isAtEndstop())
            {
                goalStep ++;
                moveOneStep(Direction::Backward);
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
                        moveOneStep(Direction::Backward); // take extra steps to reach endstopExtraDistance 
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
                enableStepper();
                moveOneStep(Direction::Forward);
                currentStep = currentStep.value() + 1;
                _state = MotionVisorState::Opening;
            } 
            else if(currentStep.value() > goalStep and !isAtEndstop()) 
            {
                enableStepper();
                moveOneStep(Direction::Backward);
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
                    enableStepper();
                    moveOneStep(Direction::Backward);
                }
            }
            else 
            {
                if(_state != MotionVisorState::Error) 
                {
                    _state = MotionVisorState::Idle;
                }
                disableStepper();
            }
            cnt = 0;
        }
    }
    else
    {
        disableStepper();
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
                    {
                        _state = MotionVisorState::Error; // it should be opened but endstop is sensing a closed state
                        currentStep = std::nullopt;
                    }
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
                    {
                        _state = MotionVisorState::Error; // it should be closed but endstop ain't sensing a closed state
                        currentStep = std::nullopt;
                    }
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
            currentStep = std::nullopt;
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

void MotionVisor::disableStepper()
{
    digitalWrite(enPin, HIGH);
}

void MotionVisor::enableStepper()
{
    digitalWrite(enPin, LOW);
}

void MotionVisor::moveOneStep(Direction direction)
{
    if(direction == Direction::Forward)
        digitalWrite(dirPin, config.invertDir ? HIGH : LOW);
    else
        digitalWrite(dirPin, config.invertDir ? LOW : HIGH);
    digitalWrite(stepPin, HIGH); // rotate 1 step
    digitalWrite(stepPin, LOW);
}
