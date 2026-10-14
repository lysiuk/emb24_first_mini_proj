#include <Arduino.h>

constexpr uint8_t RELAY_PIN = 13;
constexpr uint8_t CONTACT_PIN = 4;

constexpr uint8_t RELAY_ON = HIGH;
constexpr uint8_t RELAY_OFF = LOW;

constexpr int TEST_COUNT = 10;

constexpr uint32_t DEBOUNCE_US = 8000;
constexpr uint32_t TIMEOUT_US = 100000;

volatile uint32_t firstEdgeUs = 0;
volatile uint32_t lastEdgeUs = 0;

volatile bool edgeDetected = false;
volatile bool measuring = false;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

uint32_t onTimes[TEST_COUNT];
uint32_t offTimes[TEST_COUNT];

// Contact interrupt
void IRAM_ATTR contactISR() {
  uint32_t now = micros();

  portENTER_CRITICAL_ISR(&mux);

  if (measuring) {
    if (!edgeDetected) {
      firstEdgeUs = now;
      edgeDetected = true;
    }

    lastEdgeUs = now;
  }

  portEXIT_CRITICAL_ISR(&mux);
}

// Measure first contact movement,
// accepting only a stable target state.
uint32_t measureRelay(bool turnOn) {
  const uint8_t expectedContact = turnOn ? LOW : HIGH;

  portENTER_CRITICAL(&mux);

  measuring = true;
  edgeDetected = false;
  firstEdgeUs = 0;
  lastEdgeUs = 0;
  
  portEXIT_CRITICAL(&mux);

  uint32_t startUs = micros();

  digitalWrite(RELAY_PIN, turnOn ? RELAY_ON : RELAY_OFF);

  while ((uint32_t)(micros() - startUs) < TIMEOUT_US) {
    bool detected;
    uint32_t first;
    uint32_t last;

    portENTER_CRITICAL(&mux);

    detected = edgeDetected;
    first = firstEdgeUs;
    last = lastEdgeUs;

    portEXIT_CRITICAL(&mux);

    if (
      detected && (uint32_t)(micros() - last) >= 
      DEBOUNCE_US && digitalRead(CONTACT_PIN) == 
      expectedContact
    ) {
      portENTER_CRITICAL(&mux);

      measuring = false;

      portEXIT_CRITICAL(&mux);

      return first - startUs;
    }

    delayMicroseconds(50);
  }

  portENTER_CRITICAL(&mux);

  measuring = false;

  portEXIT_CRITICAL(&mux);

  return UINT32_MAX;
}

void printAverage(const char* label, const uint32_t* values) {
  uint64_t sum = 0;
  int count = 0;

  for (int i = 0; i < TEST_COUNT; i++) {
    if (values[i] != UINT32_MAX) {
      sum += values[i];
      count++;
    }
  }

  Serial.print(label);

  if (count == 0) {
    Serial.println("No valid measurements");
    return;
  }

  Serial.printf(
    "%.3f ms (%d valid)\n",
    (double)sum / count / 1000.0,
    count
  );
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  digitalWrite(RELAY_PIN, RELAY_OFF);
  pinMode(RELAY_PIN, OUTPUT);

  pinMode(CONTACT_PIN, INPUT_PULLUP);

  attachInterrupt(
    digitalPinToInterrupt(CONTACT_PIN),
    contactISR,
    CHANGE
  );

  delay(1000);

  Serial.println();
  Serial.println("=== RELAY TIMING TEST ===");

  Serial.printf(
    "Initial contact: %s\n",
    digitalRead(CONTACT_PIN) == HIGH ? "OPEN" : "CLOSED"
  );

  for (int i = 0; i < TEST_COUNT; i++) {
    Serial.printf("\n--- Test %d/%d ---\n", i + 1, TEST_COUNT);

    onTimes[i] = measureRelay(true);

    if (onTimes[i] == UINT32_MAX) {
      Serial.println("ON: TIMEOUT");
    } else {
      Serial.printf("ON:  %.3f ms\n", onTimes[i] / 1000.0);
    }

    delay(500);

    offTimes[i] = measureRelay(false);

    if (offTimes[i] == UINT32_MAX) {
        Serial.println("OFF: TIMEOUT");
    } else {
        Serial.printf("OFF: %.3f ms\n", offTimes[i] / 1000.0);
    }

    delay(500);
  }

  Serial.println();
  Serial.println("===== SUMMARY =====");

  printAverage("Average ON:  ", onTimes);
  printAverage("Average OFF: ", offTimes);

  Serial.println("===================");
}

void loop() {
  // Test runs once at startup
}
