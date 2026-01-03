#define SERIAL_RX_BUFFER_SIZE 4096
#define SERIAL_TX_BUFFER_SIZE 4096
#include "MotionVisor.hpp"
#include "FusionBusSlave.hpp"
#include "HardwareSerial.h"
#include <ArduinoJson.h>
#include <optional>

void setup() 
{
    delay(500); // startup delay
    Serial.begin(38400);
    Serial.println("Application Starting...");
    Serial.println("Firmware: Baremetal ARM Cortex Processor");
    Serial.println("Developer: Mahyar Shokraeian");
    Serial.println("Device: Ventdrive");
    Serial.println("Description: Motorized Greenhouse Vent Controller");
    Serial.println("Driver: A4988");
    Serial.println("Development Date: early 2026");
    Serial.println("UUID: 123456789"); // UUID for device identification
    Serial.println("------------------------------------------\n\n");
    
    pinMode(PB12, INPUT_PULLUP); // pairing and discovery button.
    MotionVisor motionVisor;
    // motionVisor.autoHome();
    FusionBusSlave fusionBus("Ventdrive");
    fusionBus.onCommunicate([&](const std::string& json) -> std::optional<std::string>
    {
        // parse and check json validity using ArduinoJson c++
        JsonDocument doc;
        if(deserializeJson(doc, json) == DeserializationError::Ok) // successful parse (valid json)
        {
            // process json commands
            if(doc["UUID"].as<uint32_t>() == 123456789) // Only process if UUID Matches,
            {
                auto mvConfig = motionVisor.getConfig();
                doc.containsKey("acceleration") ? mvConfig.acceleration = doc["acceleration"].as<double>() : NULL;
                doc.containsKey("speed") ? mvConfig.speed = doc["speed"].as<double>() : NULL;
                doc.containsKey("length") ? mvConfig.length = doc["length"].as<double>() : NULL;
                motionVisor.setConfig(mvConfig);
                
                JsonDocument responseDoc;
                if(motionVisor.state() == MotionVisorState::Closing)
                    responseDoc["state"] = "Closing";
                if(motionVisor.state() == MotionVisorState::Opening)
                    responseDoc["state"] = "Opening";
                if(motionVisor.state() == MotionVisorState::Idle)
                    responseDoc["state"] = "Idle";
                if(motionVisor.state() == MotionVisorState::Error)
                    responseDoc["state"] = "Error";
                
                if(motionVisor.ventingPercent().has_value())
                    responseDoc["ventingPercent"] = motionVisor.ventingPercent().value();
                else
                    responseDoc["ventingPercent"] = nullptr;
                std::string response;
                serializeJsonPretty(responseDoc, response);

                if(doc.containsKey("ventingPercent"))
                    motionVisor.setVentingPercent(doc["ventingPercent"].as<int>());
                if((doc["autoHomeFlag"] | false) == true)
                    motionVisor.autoHome();
                return response;
            }
        }
        return std::nullopt;
    }); 
    fusionBus.onPair([&]() -> std::optional<std::string> 
    {
        if(!digitalRead(PB12)) // if pairing button is pushed
        {
            const auto uuid = 123456789;
            JsonDocument doc;
            doc["UUID"] = uuid;
            std::string response;
            serializeJson(doc, response);
            return response;
        }
        return std::nullopt;
    });
    fusionBus.begin(38400);

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