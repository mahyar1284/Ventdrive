#include "SystemFacade.hpp"
#include "ArduinoJson.h"

#define PAIR_BTN PB12
#define COM_LED PB3
#define LOOP_LED PC13

SystemFacade::SystemFacade(uint32_t id):
    id(id), 
    fusionBus("Ventdrive"), 
    motionVisor(), 
    loopLedMillis(0)
{}

void SystemFacade::begin()
{
    Serial.begin(38400);
    Serial.println("Application Starting...");
    Serial.println("Firmware: Baremetal ARM Cortex Processor");
    Serial.println("Developer: Mahyar Shokraeian");
    Serial.println("Device: Ventdrive");
    Serial.println("Description: Motorized Greenhouse Vent Controller");
    Serial.println("Driver: A4988");
    Serial.println("Development Date: Jan 2026");
    Serial.println("id: 123456789"); // id for device identification
    Serial.println("------------------------------------------\n\n");

    pinMode(PAIR_BTN, INPUT_PULLUP); // pairing and discovery button.
    pinMode(LOOP_LED, OUTPUT);
    pinMode(COM_LED, OUTPUT);
    digitalWrite(LOOP_LED, LOW); // LED on

    fusionBus.onCommunicate([&](const std::string& json) -> std::optional<std::string>
    {
        // parse and check json validity using ArduinoJson c++
        JsonDocument doc;
        if(deserializeJson(doc, json) == DeserializationError::Ok) // successful parse (valid json)
        {
            Serial.println((std::string("id = ") + std::to_string(doc["id"].as<uint32_t>())).c_str());
            // process json commands
            if(doc["id"].as<uint32_t>() == id) // Only process if id Matches,
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
                motionVisor.setConfig(mvConfig);
                
                if((doc["autoHomeFlag"] | false) == true) motionVisor.autoHome();
                
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
}

void SystemFacade::loop()
{
    fusionBus.loop();
    digitalWrite(COM_LED, LOW);
    motionVisor.loop();
    if(loopLedMillis + 1000 < millis())
    {
        digitalToggle(LOOP_LED);
        loopLedMillis = millis();
    }
}

SystemFacade::~SystemFacade() {}
