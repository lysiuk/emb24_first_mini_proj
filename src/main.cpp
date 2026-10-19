#include <Arduino.h>

#include "barrier.h"
#include "distance_sensor.h"
#include "traffic_light.h"

constexpr uint8_t SERVO_PIN = 47;

constexpr uint8_t TRIG_PIN = 5;
constexpr uint8_t ECHO_PIN = 6;

Barrier barrier(SERVO_PIN);
DistanceSensor sensor(TRIG_PIN, ECHO_PIN);

TrafficLight trafficLight(4, 7, 13, 0, barrier, sensor);

void setup() {
  Serial.begin(115200);

  sensor.init();
  barrier.init();
  trafficLight.init();
}

void loop() {
  sensor.update();
  trafficLight.update();
  barrier.update();
}
