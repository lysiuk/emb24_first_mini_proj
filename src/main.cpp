#include <Arduino.h>
#include <cstdint>

#define LDR_PIN    4
#define RELAY_PIN 13
#define BOOT_PIN   0
#define LED_PIN RGB_BUILTIN

const uint16_t THRESHOLD_DARK  = 1800;
const uint16_t THRESHOLD_LIGHT = 2400;
const uint8_t LED_BRIGHTNESS = 30;

enum WorkMode {
  MODE_AUTO,
  MODE_ALWAYS_ON,
  MODE_ALWAYS_OFF
};

WorkMode currentMode = MODE_AUTO;

bool relayState = false;
bool lastButtonReading = HIGH;
bool buttonState = HIGH;

const uint8_t DEBOUNCE_DELAY = 50;

uint32_t lastDebounceTime = 0;
uint32_t lastPrintTime = 0;


// =====================================================
// RGB LED
// =====================================================
void setModeLed() {
  switch (currentMode) {
    // Auto -> green
    case MODE_AUTO:
      neopixelWrite(
        LED_PIN,
        0,
        LED_BRIGHTNESS,
        0
      );
      break;
    // Always ON -> blue
    case MODE_ALWAYS_ON:
      neopixelWrite(
        LED_PIN,
        0,
        0,
        LED_BRIGHTNESS
      );
      break;
    // Off -> red
    case MODE_ALWAYS_OFF:
      neopixelWrite(
        LED_PIN,
        LED_BRIGHTNESS,
        0,
        0
      );
      break;
  }
}

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
// Mode name
// =====================================================
const char* getModeName() {
  switch (currentMode) {
    case MODE_AUTO:
      return "AUTO";
    case MODE_ALWAYS_ON:
      return "ALWAYS ON";
    case MODE_ALWAYS_OFF:
      return "ALWAYS OFF";
  }

  return "UNKNOWN";
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

  setModeLed();

  Serial.println();
  Serial.print("MODE -> ");
  Serial.println(getModeName());
  Serial.println();
}

// =====================================================
// Boot btn control
// =====================================================
void handleButton() {
  bool reading = digitalRead(BOOT_PIN);

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
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BOOT_PIN, INPUT_PULLUP);
  pinMode(LDR_PIN, INPUT);

  analogReadResolution(12);
  analogSetPinAttenuation(LDR_PIN, ADC_11db);

  setRelay(false);

  currentMode = MODE_AUTO;

  setModeLed();

  Serial.println();
  Serial.println("==========================");
  Serial.println(" Light Relay Controller");
  Serial.println("==========================");
  Serial.println();

  Serial.println("Mode: AUTO");
}

void loop() {
  handleButton();

  int adcValue = analogRead(LDR_PIN);

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

  if (millis() - lastPrintTime >= 500) {
    lastPrintTime = millis();

    Serial.print("ADC: ");
    Serial.print(adcValue);

    Serial.print(" | Mode: ");
    Serial.print(getModeName());

    Serial.print(" | Relay: ");
    Serial.println(relayState ? "ON" : "OFF");
  }
}