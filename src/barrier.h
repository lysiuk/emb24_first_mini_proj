#pragma once

#include <Arduino.h>
#include <ESP32Servo.h>

class Barrier {
    private:
        Servo _servo;
        uint8_t _pin;

        static constexpr uint8_t _openAngle = 90;
        static constexpr uint8_t _closedAngle = 0;

        static constexpr uint32_t _stepInterval = 10;

        int _currentAngle = _openAngle;
        int _targetAngle = _openAngle;

        uint32_t _lastStepTime = 0;
    public:
        explicit Barrier(uint8_t pin) : _pin(pin) {}

        void init() {
            _servo.setPeriodHertz(50);
            _servo.attach(_pin, 1000, 2000);
            _servo.write(_currentAngle);
        }

        void open() {
            _targetAngle = _openAngle;
        }

        void close() {
            _targetAngle = _closedAngle;
        }

        void update() {
            const uint32_t now = millis();

            if (now - _lastStepTime < _stepInterval) {
                return;
            }

            _lastStepTime = now;

            if (_currentAngle < _targetAngle) {
                ++_currentAngle;
            } else if (_currentAngle > _targetAngle) {
                --_currentAngle;
            } else {
                return;
            }

            _servo.write(_currentAngle);
        }
};
