#include <Arduino.h>
#include <esp_arduino_version.h>

constexpr uint8_t BUZZER_PIN = 13;
constexpr uint8_t PWM_CHANNEL = 0;

// Notes in Hz
constexpr uint16_t C5  = 523;
constexpr uint16_t D5  = 587;
constexpr uint16_t Eb5 = 622;
constexpr uint16_t F5  = 698;
constexpr uint16_t G5  = 784;
constexpr uint16_t A5_1  = 880;
constexpr uint16_t Ab5 = 831;
constexpr uint16_t Bb5 = 932;
constexpr uint16_t C6  = 1047;

struct Note {
    uint16_t frequency;
    uint8_t frames;
};

// Battle City - Stage Start
const Note melody[] = {
    {C5, 8}, {D5, 8}, {Eb5, 8},
    {C5, 8}, {D5, 8}, {Eb5, 8},

    {Eb5, 8}, {F5, 8}, {G5, 8},
    {Eb5, 8}, {F5, 8}, {G5, 8},

    {F5, 8}, {G5, 8}, {A5_1, 8},
    {F5, 8}, {G5, 8}, {A5_1, 8},

    {Ab5, 8}, {Bb5, 8}, {C6, 8},
    {Ab5, 8}, {Bb5, 8}, {C6, 8},

    {C6, 24}, {C6, 8}, {C6, 8},
    {C6, 8}, {C6, 24}
};

constexpr size_t NOTE_COUNT = sizeof(melody) / sizeof(melody[0]);

constexpr uint32_t REPEAT_PAUSE = 1500;
constexpr uint8_t GATE_PERCENT = 92;

size_t noteIndex = 0;
uint32_t noteStart = 0;
uint32_t noteDuration = 0;

enum class PlayerState {
    START,
    PLAY,
    GAP,
    PAUSE
};

PlayerState state = PlayerState::START;

void playTone(uint16_t frequency) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWriteTone(BUZZER_PIN, frequency);
#else
    ledcWriteTone(PWM_CHANNEL, frequency);
#endif
}

void setup() {
    Serial.begin(115200);

#if ESP_ARDUINO_VERSION_MAJOR >= 3
    if (!ledcAttach(BUZZER_PIN, 2000, 8)) {
        Serial.println("LEDC init failed");
        state = PlayerState::PAUSE;
        return;
    }
#else
    ledcSetup(PWM_CHANNEL, 2000, 8);
    ledcAttachPin(BUZZER_PIN, PWM_CHANNEL);
#endif

    playTone(0);
    Serial.println("Battle City - Stage Start");
}

void loop() {
    const uint32_t now = millis();

    switch (state) {
        case PlayerState::START: {
            const Note& note = melody[noteIndex];
            noteDuration = note.frames * 1000UL / 60;

            playTone(note.frequency);
            noteStart = now;
            state = PlayerState::PLAY;
            break;
        }

        case PlayerState::PLAY:
            if (now - noteStart >=
                noteDuration * GATE_PERCENT / 100) {
                playTone(0);
                state = PlayerState::GAP;
            }
            break;

        case PlayerState::GAP:
            if (now - noteStart >= noteDuration) {
                noteIndex++;

                if (noteIndex >= NOTE_COUNT) {
                    noteStart = now;
                    state = PlayerState::PAUSE;
                } else {
                    state = PlayerState::START;
                }
            }
            break;

        case PlayerState::PAUSE:
            if (now - noteStart >= REPEAT_PAUSE) {
                noteIndex = 0;
                state = PlayerState::START;
            }
            break;
    }
}
