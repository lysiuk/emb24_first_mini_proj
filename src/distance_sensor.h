#pragma once

#include <Arduino.h>

class DistanceSensor {
    private:
        enum class State {
            IDLE,
            TRIGGER_LOW,
            TRIGGER_HIGH,
            WAIT_ECHO_HIGH,
            WAIT_ECHO_LOW
        };

        uint8_t _trigPin;
        uint8_t _echoPin;

        static constexpr float _detectDistance = 5.0f;
        static constexpr float _releaseDistance = 7.0f;

        static constexpr uint8_t _requiredReadings = 3;

        static constexpr uint32_t _measureInterval = 70000;
        static constexpr uint32_t _echoTimeout = 10000;

        State _state = State::IDLE;

        uint32_t _lastMeasureTime = 0;
        uint32_t _stateStartTime = 0;
        uint32_t _echoStartTime = 0;

        float _distance = -1.0f;
        bool _objectDetected = false;

        uint8_t _detectCount = 0;
        uint8_t _releaseCount = 0;

        void _processDistance(float distance);
    public:
        DistanceSensor(uint8_t trigPin, uint8_t echoPin);

        void init();
        void update();

        float getDistance() const;
        bool isObjectDetected() const;
};
