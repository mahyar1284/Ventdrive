#define SERIAL_RX_BUFFER_SIZE 4096
#define SERIAL_TX_BUFFER_SIZE 4096
#include "MotionVisor.hpp"
#include "FusionBusSlave.hpp"
#include "HardwareSerial.h"
#include <ArduinoJson.h>

void setup() 
{
    delay(500); // startup delay
    Serial.begin(115200);
    Serial.println("Application Starting...");
    Serial.println("Firmware: Baremetal ARM Cortex Processor");
    Serial.println("Developer: Mahyar Shokraeian");
    Serial.println("Device: Ventdrive");
    Serial.println("Description: Motorized Greenhouse Vent Controller");
    Serial.println("Driver: A4988");
    Serial.println("Development Date: early 2026");
    Serial.println("UUID: 123456789"); // UUID for device identification
    Serial.println("------------------------------------------\n\n");
    
    MotionVisor motionVisor;
    FusionBusSlave fusionBus("Ventdrive", [&](const std::string& json) -> std::string 
    {
        // parse and check json validity using ArduinoJson c++
        JsonDocument doc;
        if(deserializeJson(doc, json) == DeserializationError::Ok) // successful parse (valid json)
        {
            // process json commands
            if(doc["UUID"].as<uint32_t>() == 123456789) // if UUID Matches,
            {
                auto mvConfig = MotionVisorConfig();
                doc.containsKey("speed") ? mvConfig.speed = doc["speed"].as<double>() : NULL;
                doc.containsKey("length") ? mvConfig.length = doc["length"].as<double>() : NULL;
                motionVisor.setConfig(mvConfig);
                std::string command = doc["command"].as<std::string>();                
                Serial.println(command.c_str());
                if(command == "close")
                {
                    // close the visor
                    motionVisor.close();
                    return std::string("Closing visor...\n");
                }
                else if(command == "open")
                {
                    // open the visor
                    motionVisor.open();
                    return std::string("Opening visor...\n");
                }
                else
                {
                    return std::string("Error: Unknown command\n");
                }
            }
        }
        return std::string("");
    }); 
    fusionBus.begin();

    // if(motionVisor.state() == MotionVisorState::Closed)
    //     motionVisor.open();
    // if(motionVisor.state() == MotionVisorState::Opened)
    //     motionVisor.close();
    
    pinMode(PC13, OUTPUT);
    digitalWrite(PC13, LOW); // onboard LED on
    while(true)
    {
        fusionBus.loop();
        motionVisor.loop();
        digitalToggle(PC13);
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
        //delay(1);
    }
}
void loop() {}