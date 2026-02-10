#pragma once
#include <Arduino.h>
#include <string>
#include <functional>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <HardwareSerial.h>
#include <optional>

class FusionBusSlave 
{
public:
    enum class State 
    {
        Idle,           // Waiting for "FusionBus"
        WaitCommand,    // After "FusionBus", waiting for "Pair" or "Communicate"
        WaitJsonStart,  // After "FusionBusCommunicate", waiting for '{'
        CaptureJson,    // Capturing JSON until matching '}'
        Respond         // Sending response (Pair or callback result)
    };
    using Callback = std::function<std::optional<std::string>(const std::string&)>;
    using PairingCallback = std::function<std::optional<std::string>()>;

    struct Timeouts 
    {
        unsigned long primaryTriggerMs   = 150;
        unsigned long commandTriggerMs   = 150;
        unsigned long jsonStartMs        = 150;
        unsigned long jsonCompleteMs     = 250;
    };

    FusionBusSlave(std::string deviceType = "Ventdrive",
                   Callback onCommunicate = {})
        : serial_(HardwareSerial(USART1)),
          deviceType_(std::move(deviceType)),
          timeouts_(),
          onCommunicate_(std::move(onCommunicate))
    {}

    void begin(unsigned long baud = 115200) 
    {
        serial_.begin(baud);
        USART1->CR3 |= USART_CR3_HDSEL;
        serial_.println("abcdefghijklmnopqrstuvwxyz1234567890{}[]()!@#$%^&*~,.-_/''<>ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        reset();
        // Serial.println("[FusionBusSlave] begin() called, state reset to Idle");
    }

    void loop() 
    {
        uartRecoverIfNeeded();
        while (serial_.available()) 
        {
            const char c = static_cast<char>(serial_.read());
            processChar(c);
        }
        checkTimeouts();
        flushRespondIfPending();
    }

    static inline void uartRecoverIfNeeded() 
    {
        uint32_t sr = USART1->SR;

        if (sr & (USART_SR_ORE | USART_SR_FE | USART_SR_NE | USART_SR_PE)) 
        {
            volatile uint32_t dummy;
            dummy = sr;           // read SR first
            dummy = USART1->DR;   // read DR clears the error
            (void)dummy;
        }
    }


    void onCommunicate(Callback cb) 
    {
        onCommunicate_ = std::move(cb);
    }

    void onPair(PairingCallback cb) 
    {
        onPair_ = std::move(cb);
    }

    void setDeviceType(std::string type) 
    {
        deviceType_ = std::move(type);
    }

    void setTimeouts(const Timeouts& t) 
    {
        timeouts_ = t;
    }

private:
    HardwareSerial serial_;
    std::string deviceType_;
    Timeouts timeouts_;
    Callback onCommunicate_;
    PairingCallback onPair_;

    State state_ = State::Idle;

    std::string tokenBuffer_;
    std::string jsonBuffer_;

    unsigned long stateStartMs_ = 0;
    int braceDepth_ = 0;

    std::string pendingResponse_;
    bool hasPendingResponse_ = false;

    static constexpr const char* kPrimary = "FusionBus";
    static constexpr const char* kPair    = "Pair";
    static constexpr const char* kComm    = "Communicate";

    void processChar(char c) 
    {
        switch (state_) 
        {
            case State::Idle:          handleIdle(c); break;
            case State::WaitCommand:   handleWaitCommand(c); break;
            case State::WaitJsonStart: handleWaitJsonStart(c); break;
            case State::CaptureJson:   handleCaptureJson(c); break;
            case State::Respond:       break;
        }
    }

    void handleIdle(char c) 
    {
        if (std::isspace(static_cast<unsigned char>(c))) return;
        tokenBuffer_.push_back(c);

        if (startsWith(kPrimary)) 
        {
            if (tokenBuffer_.size() == std::strlen(kPrimary)) 
            {
                // Serial.println("[FusionBusSlave] Primary trigger matched: FusionBus");
                transition(State::WaitCommand);
                tokenBuffer_.clear();
            }
        } else 
        {
            if (c == kPrimary[0]) 
            {
                tokenBuffer_.assign(1, c);
            } else 
            {
                tokenBuffer_.clear();
            }
        }
    }

    void handleWaitCommand(char c) 
    {
        if (std::isspace(static_cast<unsigned char>(c))) return;
        tokenBuffer_.push_back(c);

        if (startsWith(kPair)) 
        {
            if (tokenBuffer_.size() == std::strlen(kPair)) 
            {
                // Serial.println("[FusionBusSlave] Command trigger matched: Pair");
                if (onPair_) 
                {
                    auto optResponse = onPair_();
                    if(optResponse.has_value())
                    {
                        pendingResponse_ = optResponse.value();
                        hasPendingResponse_ = true;
                        delay(10); // wait 10ms to avoid bus collision
                        transition(State::Respond);
                    }
                    else
                    {
                        reset();
                    }
                }
                return;
            }
        } else if (startsWith(kComm)) 
        {
            if (tokenBuffer_.size() == std::strlen(kComm)) 
            {
                // Serial.println("[FusionBusSlave] Command trigger matched: Communicate");
                transition(State::WaitJsonStart);
                tokenBuffer_.clear();
                return;
            }
        } else 
        {
            if (c == kPair[0]) 
            {
                tokenBuffer_.assign(1, c);
            } else if (c == kComm[0]) 
            {
                tokenBuffer_.assign(1, c);
            } else 
            {
                tokenBuffer_.clear();
            }
        }
    }

    void handleWaitJsonStart(char c) 
    {
        if (c == '{') 
        {
            // Serial.println("[FusionBusSlave] JSON start detected");
            jsonBuffer_.clear();
            jsonBuffer_.push_back(c);
            braceDepth_ = 1;
            transition(State::CaptureJson);
        }
    }

    void handleCaptureJson(char c) 
    {
        jsonBuffer_.push_back(c);
        if (c == '{') 
        {
            ++braceDepth_;
        } else if (c == '}') 
        {
            --braceDepth_;
            if (braceDepth_ <= 0) 
            {
                // Serial.print("[FusionBusSlave] JSON captured: ");
                // Serial.println(jsonBuffer_.c_str());
                if (onCommunicate_) 
                {
                    auto optResponse = onCommunicate_(jsonBuffer_);
                    if(optResponse.has_value())
                    {
                        pendingResponse_ = optResponse.value();
                        hasPendingResponse_ = true;
                        delay(10); // wait to avoid bus collision
                        transition(State::Respond);
                    }
                    else
                    {
                        reset();
                    }
                }
            }
        }
    }

    bool startsWith(const char* target) const 
    {
        const size_t n = tokenBuffer_.size();
        const size_t m = std::strlen(target);
        if (n > m) return false;
        for (size_t i = 0; i < n; ++i) 
        {
            if (tokenBuffer_[i] != target[i]) return false;
        }
        return true;
    }

    void transition(State next) 
    {
        // Serial.print("[FusionBusSlave] Transition: ");
        // Serial.print(static_cast<int>(state_));
        // Serial.print(" -> ");
        // Serial.println(static_cast<int>(next));
        state_ = next;
        stateStartMs_ = millis();
    }

    void reset() 
    {
        // Serial.println("[FusionBusSlave] Resetting state to Idle");
        state_ = State::Idle;
        tokenBuffer_.clear();
        jsonBuffer_.clear();
        braceDepth_ = 0;
        hasPendingResponse_ = false;
        pendingResponse_.clear();
        stateStartMs_ = millis();
    }

    void checkTimeouts() 
    {
        const unsigned long now = millis();
        switch (state_) 
        {
            case State::Idle:
                if (elapsed(now) > timeouts_.primaryTriggerMs) 
                {
                    tokenBuffer_.clear();
                    stateStartMs_ = now;
                }
                break;
            case State::WaitCommand:
                if (elapsed(now) > timeouts_.commandTriggerMs) 
                {
                    // Serial.println("[FusionBusSlave] Timeout in WaitCommand");
                    reset();
                }
                break;
            case State::WaitJsonStart:
                if (elapsed(now) > timeouts_.jsonStartMs) 
                {
                    // Serial.println("[FusionBusSlave] Timeout waiting for JSON start");
                    reset();
                }
                break;
            case State::CaptureJson:
                if (elapsed(now) > timeouts_.jsonCompleteMs) 
                {
                    // Serial.println("[FusionBusSlave] Timeout capturing JSON");
                    reset();
                }
                break;
            case State::Respond:
                break;
        }
    }

    unsigned long elapsed(unsigned long now) const 
    {
        return now - stateStartMs_;
    }

    void flushRespondIfPending() 
    {
        if (state_ == State::Respond && hasPendingResponse_) 
        {
            // Serial.print("[FusionBusSlave] Sending response: ");
            // Serial.println(pendingResponse_.c_str());
            serial_.flush();
            if(serial_.availableForWrite())
            {
                serial_.println(pendingResponse_.c_str());
                hasPendingResponse_ = false;
                pendingResponse_.clear();
                reset();
            }
        }
    }

    std::string makeUuid() const 
    {
        const uint32_t a = millis();
        const uint32_t b = micros();
        const uintptr_t p = reinterpret_cast<uintptr_t>(this);
        char buf[33] = {0};
        snprintf(buf, sizeof(buf), "%08lx%08lx%08lx",
                 static_cast<unsigned long>(a),
                 static_cast<unsigned long>(b),
                 static_cast<unsigned long>(p & 0xFFFFFFFFUL));
        return std::string(buf);
    }
};