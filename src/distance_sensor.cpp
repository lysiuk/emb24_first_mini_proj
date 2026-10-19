#include "distance_sensor.h"

DistanceSensor::DistanceSensor(
    uint8_t trigPin,
    uint8_t echoPin
) : _trigPin(trigPin),
    _echoPin(echoPin) {}

void DistanceSensor::init() {
    pinMode(_trigPin, OUTPUT);
    pinMode(_echoPin, INPUT);

    digitalWrite(_trigPin, LOW);

    _lastMeasureTime = micros() - _measureInterval;
}

void DistanceSensor::_processDistance(float distance) {
    _distance = distance;

    if (distance <= _detectDistance) {
        _detectCount++;
        _releaseCount = 0;

        if (_detectCount >= _requiredReadings) {
            _detectCount = _requiredReadings;
            _objectDetected = true;
        }
    } else if (distance >= _releaseDistance) {
        _releaseCount++;
        _detectCount = 0;

        if (_releaseCount >= _requiredReadings) {
            _releaseCount = _requiredReadings;
            _objectDetected = false;
        }
    } else {
        _detectCount = 0;
        _releaseCount = 0;
    }
}

void DistanceSensor::update() {
    const uint32_t now = micros();

    switch (_state) {
        case State::IDLE:
            if (now - _lastMeasureTime >= _measureInterval) {
                digitalWrite(_trigPin, LOW);
                _stateStartTime = now;
                _state = State::TRIGGER_LOW;
            }
            break;

        case State::TRIGGER_LOW:
            if (now - _stateStartTime >= 2) {
                digitalWrite(_trigPin, HIGH);
                _stateStartTime = micros();
                _state = State::TRIGGER_HIGH;
            }
            break;

        case State::TRIGGER_HIGH:
            if (now - _stateStartTime >= 10) {
                digitalWrite(_trigPin, LOW);
                _stateStartTime = micros();
                _state = State::WAIT_ECHO_HIGH;
            }
            break;

        case State::WAIT_ECHO_HIGH:
            if (digitalRead(_echoPin) == HIGH) {
                _echoStartTime = micros();
                _state = State::WAIT_ECHO_LOW;
            } else if (now - _stateStartTime >= _echoTimeout) {
                _distance = -1.0f;
                _detectCount = 0;
                _releaseCount = 0;
                _lastMeasureTime = now;
                _state = State::IDLE;
            }
            break;

        case State::WAIT_ECHO_LOW:
            if (digitalRead(_echoPin) == LOW) {
                const uint32_t duration =
                    micros() - _echoStartTime;

                const float distance =
                    duration * 0.0343f / 2.0f;

                _processDistance(distance);

                _lastMeasureTime = micros();
                _state = State::IDLE;
            } else if (now - _echoStartTime >= _echoTimeout) {
                _distance = -1.0f;
                _detectCount = 0;
                _releaseCount = 0;
                _lastMeasureTime = now;
                _state = State::IDLE;
            }
            break;
    }
}

float DistanceSensor::getDistance() const {
    return _distance;
}

bool DistanceSensor::isObjectDetected() const {
    return _objectDetected;
}
