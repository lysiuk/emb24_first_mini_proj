#include <Arduino.h>
#include <cstdint>

#define LED_PIN RGB_BUILTIN

enum WorkMode {
  MODE_AUTO,
  MODE_ALWAYS_ON,
  MODE_ALWAYS_OFF
};

class LedRgb {
  private:
    uint8_t _pin;
    uint8_t _brightness;
  public:
    LedRgb(uint8_t pin, uint8_t brightness = 30)
      : _pin(pin),
      _brightness(brightness) {
    }

    void init() {
      neopixelWrite(_pin, 0, _brightness, 0);
    }

    void set(WorkMode state) {
      switch (state) {
      // AUTO -> green
      case MODE_AUTO:
          neopixelWrite(_pin, 0, _brightness, 0);
          break;
      // ALWAYS_ON -> blue
      case MODE_ALWAYS_ON:
          neopixelWrite(_pin, 0, 0, _brightness);
          break;
      // ALWAYS_OFF -> red
      case MODE_ALWAYS_OFF:
          neopixelWrite(_pin, _brightness, 0, 0);
          break;
      }
    }
};

constexpr uint8_t LDR_PIN = 4;
constexpr uint8_t RELAY_PIN = 13;
constexpr uint8_t BOOT_PIN = 0;
constexpr uint16_t THRESHOLD_DARK  = 1800;
constexpr uint16_t THRESHOLD_LIGHT = 2400;
constexpr uint8_t LED_BRIGHTNESS = 30;

LedRgb ledRgb(RGB_BUILTIN);
WorkMode currentMode = MODE_AUTO;

bool relayState = false;
bool lastButtonReading = HIGH;
bool buttonState = HIGH;

const uint8_t DEBOUNCE_DELAY = 50;

uint32_t lastDebounceTime = 0;

// =====================================================
// Relay control
// =====================================================
void setRelay(bool state) {
  relayState = state;

  digitalWrite(
    RELAY_PIN,
    relayState ? HIGH : LOW
  );
}

// =====================================================
// Mode switch
// =====================================================
void nextMode() {
  switch (currentMode) {
    case MODE_AUTO:
      currentMode = MODE_ALWAYS_ON;
      break;
    case MODE_ALWAYS_ON:
      currentMode = MODE_ALWAYS_OFF;
      break;
    case MODE_ALWAYS_OFF:
      currentMode = MODE_AUTO;
      break;
  }

  ledRgb.set(currentMode);
}

// =====================================================
// Boot btn control
// =====================================================
void handleButton() {
  const bool reading = digitalRead(BOOT_PIN);

  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
    lastButtonReading = reading;
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        nextMode();
      }
    }
  }
}

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BOOT_PIN, INPUT_PULLUP);
  pinMode(LDR_PIN, INPUT);

  analogReadResolution(12);
  analogSetPinAttenuation(LDR_PIN, ADC_11db);

  setRelay(false);

  ledRgb.init();
  currentMode = MODE_AUTO;
  ledRgb.set(currentMode);
}

void loop() {
  handleButton();

  const int adcValue = analogRead(LDR_PIN);

  switch (currentMode) {
    case MODE_AUTO:
      if (adcValue < THRESHOLD_DARK) {
        setRelay(true);
      }
      else if (adcValue > THRESHOLD_LIGHT) {
        setRelay(false);
      }
      break;
    case MODE_ALWAYS_ON:
      setRelay(true);
      break;
    case MODE_ALWAYS_OFF:
      setRelay(false);
      break;
  }
}
