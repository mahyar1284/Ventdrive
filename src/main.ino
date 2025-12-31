#define SERIAL_RX_BUFFER_SIZE 4096
#define SERIAL_TX_BUFFER_SIZE 4096
#include "MotionVisor.hpp"
#include "FusionBusSlave.hpp"
#include "HardwareSerial.h"

void setup() 
{
    Serial.begin(115200);
    Serial.println("Application Starting...");
    Serial.println("Firmware: Baremetal ARM Cortex Processor");
    Serial.println("Developer: Mahyar Shokraeian");
    Serial.println("Device: Ventdrive");
    Serial.println("Description: Motorized Greenhouse Vent Controller");
    Serial.println("Driver: A4988");
    Serial.println("Development Date: early 2026");
    
    FusionBusSlave fusionBus("Ventdrive", [](const std::string& json) -> std::string 
    {
        // Parse or route the JSON; return a response string
        // For demo, echo with an ack field
        return std::string("{\"ack\":true,\"received\":") + json + "}";
    });

    fusionBus.begin();

    pinMode(PC13, OUTPUT);
    digitalWrite(PC13, LOW); // onboard LED on
    MotionVisor motionVisor;
    if(motionVisor.state() == MotionVisorState::Closed)
        motionVisor.open();
    if(motionVisor.state() == MotionVisorState::Opened)
        motionVisor.close();
    while(true)
    {
        fusionBus.loop();
        motionVisor.loop();

        // if(motionVisor.state() == MotionVisorState::Closed)
        //     Serial.println("state: Closed");
        // if(motionVisor.state() == MotionVisorState::Opened)
        //     Serial.println("state: Opened");
        // if(motionVisor.state() == MotionVisorState::Closing)
        //     Serial.println("state: Closing");
        // if(motionVisor.state() == MotionVisorState::Opening)
        //     Serial.println("state: Opening");
        // if(motionVisor.state() == MotionVisorState::Error)
        //     Serial.println("state: Error");
        // sSerial.println("LOOP\n");
        delay(1);
    }
}
void loop() {}