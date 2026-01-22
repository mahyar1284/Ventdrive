#define SERIAL_RX_BUFFER_SIZE 4096
#define SERIAL_TX_BUFFER_SIZE 4096
#include "MotionVisor.hpp"
#include "FusionBusSlave.hpp"
#include "HardwareSerial.h"
#include <ArduinoJson.h>
#include <optional>

#define PAIR_BTN PB12
#define COM_LED PB3
#define LOOP_LED PC13

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
    Serial.println("id: 123456789"); // id for device identification
    Serial.println("------------------------------------------\n\n");
    
    pinMode(PAIR_BTN, INPUT_PULLUP); // pairing and discovery button.
    pinMode(LOOP_LED, OUTPUT);
    pinMode(COM_LED, OUTPUT);
    digitalWrite(LOOP_LED, LOW); // LED on
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
            if(doc["id"].as<uint32_t>() == 123456789) // Only process if id Matches,
            {
                digitalWrite(COM_LED, HIGH);
                auto mvConfig = motionVisor.getConfig();
                if(doc.containsKey("acceleration")) mvConfig.acceleration = doc["acceleration"].as<double>();
                if(doc.containsKey("speed")) mvConfig.speed = doc["speed"].as<double>();
                if(doc.containsKey("length")) mvConfig.length = doc["length"].as<double>();
                if(doc.containsKey("stepPermm")) mvConfig.stepPermm = doc["stepPermm"].as<double>();
                if(doc.containsKey("maxCompensation")) mvConfig.maxCompensation = doc["maxCompensation"].as<double>();
                if(doc.containsKey("endstopExtraDistance")) mvConfig.endstopExtraDistance = doc["endstopExtraDistance"].as<double>();
                if(doc.containsKey("ventingPercent")) motionVisor.setVentingPercent(doc["ventingPercent"].as<int>());
                if(doc.containsKey("invertDir")) mvConfig.invertDir = doc["invertDir"].as<bool>();
                if(doc.containsKey("invertEndstopPin")) mvConfig.invertEndstopPin = doc["invertEndstopPin"].as<bool>();
                if((doc["autoHomeFlag"] | false) == true) motionVisor.autoHome();
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
                if(motionVisor.state() == MotionVisorState::Uninitialized)
                    responseDoc["state"] = "Uninitialized";
                
                if(motionVisor.ventingPercent().has_value())
                    responseDoc["ventingPercent"] = motionVisor.ventingPercent().value();
                else
                    responseDoc["ventingPercent"] = nullptr;
                responseDoc["type"] = "VentDrive";
                std::string response;
                serializeJsonPretty(responseDoc, response);

                return response;
            }
        }
        return std::nullopt;
    }); 
    fusionBus.onPair([&]() -> std::optional<std::string> 
    {
        if(!digitalRead(PAIR_BTN)) // if pairing button is pushed
        {
            const auto id = 123456789;
            JsonDocument doc;
            doc["id"] = id;
            doc["type"] = "VentDrive";
            std::string response;
            serializeJson(doc, response);
            return response;
        }
        return std::nullopt;
    });
    fusionBus.begin(38400);

    long long loopLedMillis = 0;
    while(true)
    {
        fusionBus.loop();
        digitalWrite(COM_LED, LOW);
        motionVisor.loop();
        if(loopLedMillis + 1000 < millis())
        {
            digitalToggle(LOOP_LED);
            loopLedMillis = millis();
        }
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