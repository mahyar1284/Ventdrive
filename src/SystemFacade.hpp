#pragma once
#include "MotionVisor.hpp"
#include "FusionBusSlave.hpp"

class SystemFacade
{
public:
    SystemFacade(uint32_t id = 123456789);
    void begin();
    void loop();
    ~SystemFacade();

private:
    uint32_t id;
    FusionBusSlave fusionBus;
    MotionVisor motionVisor;
    long long loopLedMillis;
};
