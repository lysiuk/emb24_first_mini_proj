#pragma once

#include <Arduino.h>
#include "barrier.h"
#include "distance_sensor.h"

class TrafficLight {
    private:
        enum class TrafficState {
            GREEN,
            GREEN_BLINK,
            YELLOW,
            RED,
            RED_YELLOW,
            YELLOW_BLINK
        };

        enum WorkMode {
            DAY,
            NIGHT
        };

        uint8_t _redPin;
        uint8_t _yellowPin;
        uint8_t _greenPin;

        uint8_t _btnPin;

        static constexpr uint32_t _greenTime = 15000;
        static constexpr uint32_t _greenBlinkTime = 5000;
        static constexpr uint32_t _yellowTime = 5000;
        static constexpr uint32_t _redTime = 15000;
        static constexpr uint32_t _redYellowTime = 2000;

        static constexpr uint32_t _blinkInterval = 500;

        TrafficState _currentState = TrafficState::GREEN;
        WorkMode _currentMode = WorkMode::DAY;
        Barrier& _barrier;

        DistanceSensor& _sensor;
        bool _barrierClosedOnYellow = false;

        uint32_t _stateStartTime = 0;
        uint32_t _lastBlinkTime = 0;

        bool _lastButtonReading = HIGH;
        bool _buttonState = HIGH;

        uint32_t _lastDebounceTime = 0;

        static constexpr uint8_t _debounceDelay = 50;

        bool _greenLedState = HIGH;
        bool _yellowLedState = HIGH;

        void _setLeds(bool red, bool yellow, bool green);
        void _setState(TrafficState newState, uint32_t now);
        void _setWorkMode(WorkMode state, uint32_t now);
        void _nextMode(uint32_t now);
        void _handleButton(uint32_t now);
    public:
        TrafficLight(uint8_t redPin, uint8_t yellowPin, uint8_t greenPin, uint8_t btnPin, Barrier& barrier, DistanceSensor& sensor);

        void init();
        void update();
};
