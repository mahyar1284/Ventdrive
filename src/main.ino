#include "MotionVisor.hpp"

void setup() 
{
    Serial.begin(115200);
    Serial.println("hello from MotionVisor!");
    pinMode(PC13, OUTPUT);
    digitalWrite(PC13, LOW); // onboard LED on
    MotionVisor motionVisor;
    if(motionVisor.state() == MotionVisorState::Closed)
        motionVisor.open();
    if(motionVisor.state() == MotionVisorState::Opened)
        motionVisor.close();
    while(true)
    {
        motionVisor.loop();
        if(motionVisor.state() == MotionVisorState::Closed)
            Serial.println("state: Closed");
        if(motionVisor.state() == MotionVisorState::Opened)
            Serial.println("state: Opened");
        if(motionVisor.state() == MotionVisorState::Closing)
            Serial.println("state: Closing");
        if(motionVisor.state() == MotionVisorState::Opening)
            Serial.println("state: Opening");
        if(motionVisor.state() == MotionVisorState::Error)
            Serial.println("state: Error");
        delay(100);
    }
}
void loop() {}