#include "traffic_light.h"

TrafficLight::TrafficLight(uint8_t redPin, uint8_t yellowPin, uint8_t greenPin, uint8_t btnPin, Barrier& barrier, DistanceSensor& sensor)
        : _redPin(redPin),
        _yellowPin(yellowPin),
        _greenPin(greenPin),
        _btnPin(btnPin),
        _barrier(barrier),
        _sensor(sensor) {}

void TrafficLight::_setLeds(bool red, bool yellow, bool green) {
    digitalWrite(_redPin, red);
    digitalWrite(_yellowPin, yellow);
    digitalWrite(_greenPin, green);
}

void TrafficLight::_setState(TrafficState newState, uint32_t now) {
    _currentState = newState;
    _stateStartTime = now;

    switch (_currentState) {
        case TrafficState::GREEN:
            _barrier.open();
            _setLeds(LOW, LOW, HIGH);
            Serial.println("GREEN");
            break;

        case TrafficState::GREEN_BLINK:
            _barrier.open();
            _greenLedState = HIGH;
            _lastBlinkTime = now;
            _setLeds(LOW, LOW, HIGH);
            Serial.println("GREEN BLINK");
            break;

        case TrafficState::YELLOW:
            _barrierClosedOnYellow = false;
            _barrier.open();
            _setLeds(LOW, HIGH, LOW);
            Serial.println("YELLOW");
            break;

        case TrafficState::RED:
            _barrier.close();
            _setLeds(HIGH, LOW, LOW);
            Serial.println("RED");
            break;

        case TrafficState::RED_YELLOW:
            _barrier.close();
            _setLeds(HIGH, HIGH, LOW);
            Serial.println("RED + YELLOW");
            break;

        case TrafficState::YELLOW_BLINK:
            _barrier.open();
            _yellowLedState = HIGH;
            _lastBlinkTime = now;
            _setLeds(LOW, HIGH, LOW);
            Serial.println("YELLOW BLINK");
            break;
    }
}

void TrafficLight::_setWorkMode(WorkMode state, uint32_t now) {
    switch (state) {
        case DAY:
            _setState(TrafficState::GREEN, now);
            break;
            
        case NIGHT:
            _setState(TrafficState::YELLOW_BLINK, now);
            break;
      }
}

void TrafficLight::_nextMode(uint32_t now) {
    _currentMode = (_currentMode == WorkMode::DAY)
        ? WorkMode::NIGHT
        : WorkMode::DAY;

    _setWorkMode(_currentMode, now);
}

void TrafficLight::_handleButton(uint32_t now) {
    const bool reading = digitalRead(_btnPin);

    if (reading != _lastButtonReading) {
        _lastDebounceTime = now;
        _lastButtonReading = reading;
    }

    if ((now - _lastDebounceTime) > _debounceDelay) {
        if (reading != _buttonState) {
            _buttonState = reading;

            if (_buttonState == LOW) {
                _nextMode(now);
            }
        }
    }
}

void TrafficLight::init() {
    const uint32_t now = millis();

    pinMode(_redPin, OUTPUT);
    pinMode(_yellowPin, OUTPUT);
    pinMode(_greenPin, OUTPUT);

    pinMode(_btnPin, INPUT_PULLUP);

    _buttonState = digitalRead(_btnPin);
    _lastButtonReading = _buttonState;
    _lastDebounceTime = now;

    _setLeds(LOW, LOW, LOW);
    _setState(TrafficState::GREEN, now);
}

void TrafficLight::update() {
    const uint32_t now = millis();

    _handleButton(now);

    const uint32_t elapsed = now - _stateStartTime;

    switch (_currentState) {
        case TrafficState::GREEN:
            if (elapsed >= _greenTime) {
                _setState(TrafficState::GREEN_BLINK, now);
            }
            break;

        case TrafficState::GREEN_BLINK:
            if (elapsed >= _greenBlinkTime) {
                _setState(TrafficState::YELLOW, now);
            } else if (now - _lastBlinkTime >= _blinkInterval) {
                _greenLedState = !_greenLedState;
                digitalWrite(_greenPin, _greenLedState);
                _lastBlinkTime = now;
            }
            break;

        case TrafficState::YELLOW:
            if (!_barrierClosedOnYellow && _sensor.isObjectDetected()) {
                _barrier.close();
                _barrierClosedOnYellow = true;

                Serial.println("Vehicle detected! Barrier closing");
            }

            if (elapsed >= _yellowTime) {
                _setState(TrafficState::RED, now);
            }
            break;

        case TrafficState::RED:
            if (elapsed >= _redTime) {
                _setState(TrafficState::RED_YELLOW, now);
            }
            break;

        case TrafficState::RED_YELLOW:
            if (elapsed >= _redYellowTime) {
                _setState(TrafficState::GREEN, now);
            }
            break;

        case TrafficState::YELLOW_BLINK:
            if (now - _lastBlinkTime >= _blinkInterval) {
                _yellowLedState = !_yellowLedState;
                digitalWrite(_yellowPin, _yellowLedState);
                _lastBlinkTime = now;
            }
            break;
    }
}
