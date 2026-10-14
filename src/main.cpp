#include <Arduino.h>

constexpr uint8_t RED_PIN = 4;
constexpr uint8_t YELLOW_PIN = 6;
constexpr uint8_t GREEN_PIN = 13;

uint32_t last_toggle_red = 0;
constexpr uint32_t interval_red = 200;
bool red_state = LOW;

uint32_t last_toggle_yellow = 0;
constexpr uint32_t interval_yellow = 500;
bool yellow_state = LOW;

uint32_t last_toggle_green = 0;
constexpr uint32_t interval_green = 1000;
bool green_state = LOW;

void update_led_red() {
  uint32_t now = millis();

  if (now - last_toggle_red >= interval_red) {
    red_state = !red_state;
    digitalWrite(RED_PIN, red_state);
    last_toggle_red = now;
  }
}

void update_led_yellow() {
  uint32_t now = millis();

  if (now - last_toggle_yellow >= interval_yellow) {
    yellow_state = !yellow_state;
    digitalWrite(YELLOW_PIN, yellow_state);
    last_toggle_yellow = now;
  }
}

void update_led_green() {
  uint32_t now = millis();

  if (now - last_toggle_green >= interval_green) {
    green_state = !green_state;
    digitalWrite(GREEN_PIN, green_state);
    last_toggle_green = now;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(RED_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);

  digitalWrite(RED_PIN, LOW);
  digitalWrite(YELLOW_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
}

void loop() {
  update_led_red();
  update_led_yellow();
  update_led_green();
}
