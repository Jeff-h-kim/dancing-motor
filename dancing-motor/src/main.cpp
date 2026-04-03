#include <Arduino.h>
#include <ArduinoJson.h>
#include "notes.h"
#include "music_files/toothless_dancing.h"

// -------- Pin & Channel Config --------
const int NUM_BUZZERS = 6;
const int BUZZER_PINS[NUM_BUZZERS]     = { 32, 26, 27, 14, 12, 13 };
const int BUZZER_CHANNELS[NUM_BUZZERS] = {  0,  2,  3,  4,  5,  6 };

const int motorPin        = 19;
const int motorChannel    = 1;
const int motorResolution = 8;
const int motorFreq       = 1000;

const int buzzerResolution = 8;
const int buttonPin        = 33;

// -------- Buzzer State --------
struct BuzzerState {
    bool          active;
    bool          inGap;
    unsigned long startTime;
    unsigned long playTime;
    unsigned long gapTime;
};

BuzzerState buzzerStates[NUM_BUZZERS];

// -------- Note table --------
JsonDocument notes;

// -------- Song Player --------
struct SongPlayer {
    bool          playing;
    int           noteIndex;
    int           channel;
    JsonDocument* sheetMusic;
};

JsonDocument melodyDoc, bassDoc;

const int NUM_VOICES = 2;
SongPlayer voices[NUM_VOICES] = {
    { false, 0, 0, &melodyDoc },
    { false, 0, 2, &bassDoc   },
};

int interval = 500;

// -------- Button State Machine --------
bool buttonStableState = HIGH;
bool lastReading       = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// -----------------------------------------------------------------------
void setBuzzerState(int ch, bool active, bool inGap,
                    unsigned long playTime, unsigned long gapTime) {
    buzzerStates[ch].active    = active;
    buzzerStates[ch].inGap     = inGap;
    buzzerStates[ch].playTime  = playTime;
    buzzerStates[ch].gapTime   = gapTime;
    buzzerStates[ch].startTime = millis();
}

// -----------------------------------------------------------------------
void forceStopAll() {
    for (int i = 0; i < NUM_BUZZERS; i++) {
        ledcWriteTone(BUZZER_CHANNELS[i], 0);
        buzzerStates[i].active = false;
        buzzerStates[i].inGap  = false;
    }

    for (int i = 0; i < NUM_VOICES; i++) {
        voices[i].playing   = false;
        voices[i].noteIndex = 0;
    }

    ledcWrite(motorChannel, 0);
}

// -----------------------------------------------------------------------
void playNoteOnChannel(int channel, const char* note,
                       float duration, const char* type) {
    if (channel < 0 || channel >= NUM_BUZZERS) return;

    float length = 0.70f, gap = 0.30f;
    if      (strcmp(type, "stac") == 0) { length = 0.40f; gap = 0.60f; }
    else if (strcmp(type, "lega") == 0) { length = 0.90f; gap = 0.10f; }

    unsigned long totalTime = (unsigned long)(duration * interval);
    unsigned long playTime  = (unsigned long)(totalTime * length);
    unsigned long gapTime   = (unsigned long)(totalTime * gap);

    if (strcmp(note, "Rest") == 0) {
        ledcWriteTone(BUZZER_CHANNELS[channel], 0);
        setBuzzerState(channel, true, true, 0, totalTime);
        return;
    }

    int freq = notes[note]["freq"].as<int>();
    ledcWriteTone(BUZZER_CHANNELS[channel], freq);
    setBuzzerState(channel, true, false, playTime, gapTime);
}

// -----------------------------------------------------------------------
void updateBuzzers() {
    unsigned long now = millis();

    for (int ch = 0; ch < NUM_BUZZERS; ch++) {
        if (!buzzerStates[ch].active) continue;

        unsigned long elapsed = now - buzzerStates[ch].startTime;

        if (!buzzerStates[ch].inGap) {
            if (elapsed >= buzzerStates[ch].playTime) {
                ledcWriteTone(BUZZER_CHANNELS[ch], 0);
                buzzerStates[ch].inGap = true;
                buzzerStates[ch].startTime = now;
            }
        } else {
            if (elapsed >= buzzerStates[ch].gapTime) {
                buzzerStates[ch].active = false;
            }
        }
    }
}

bool channelReady(int ch) {
    return !buzzerStates[ch].active;
}

// -----------------------------------------------------------------------
void updateSongPlayer(SongPlayer& sp) {
    if (!sp.playing || sp.sheetMusic == nullptr) return;
    if (!channelReady(sp.channel)) return;

    JsonArray music_notes = (*sp.sheetMusic)["notes"];

    if (sp.noteIndex >= music_notes.size()) {
        sp.playing = false;
        return;
    }

    const char* pitch  = music_notes[sp.noteIndex]["note"];
    float       length = music_notes[sp.noteIndex]["duration"];
    const char* type   = music_notes[sp.noteIndex]["type"];

    playNoteOnChannel(sp.channel, pitch, length, type);
    sp.noteIndex++;
}

// -----------------------------------------------------------------------
void startAllVoicesClean() {
    forceStopAll();   // hard reset everything
    delay(5);         // prevents PWM glitch on ESP32

    for (int i = 0; i < NUM_VOICES; i++) {
        voices[i].playing   = true;
        voices[i].noteIndex = 0;
    }

}

// -----------------------------------------------------------------------
bool loadSong(const char* songJson, JsonDocument& location) {
    return deserializeJson(location, songJson) == DeserializationError::Ok;
}

// -----------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(1000);

    ledcSetup(motorChannel, motorFreq, motorResolution);
    ledcAttachPin(motorPin, motorChannel);

    for (int i = 0; i < NUM_BUZZERS; i++) {
        ledcSetup(BUZZER_CHANNELS[i], 2000, buzzerResolution);
        ledcAttachPin(BUZZER_PINS[i], BUZZER_CHANNELS[i]);
        buzzerStates[i].active = false;
    }

    pinMode(buttonPin, INPUT_PULLUP);

    deserializeJson(notes, NOTES_JSON);

    loadSong(MELODY_JSON, melodyDoc);
    loadSong(BASS_JSON,   bassDoc);

    int tempo = melodyDoc["tempo"];
    interval  = 60000 / tempo;
}

bool anySongPlaying() {
    for (int i = 0; i < NUM_VOICES; i++) {
        if (voices[i].playing) return true;
    }
    return false;
}
// -----------------------------------------------------------------------
void loop() {
    // -------- BUTTON HANDLING (ROBUST) --------
    bool reading = digitalRead(buttonPin);

    if (reading != lastReading) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (reading != buttonStableState) {
            buttonStableState = reading;

            if (buttonStableState == LOW) {
                Serial.println("Clean press detected");
                startAllVoicesClean();   // 🔥 interrupt + restart
            }
        }
    }

    lastReading = reading;

    // -------- AUDIO ENGINE --------
    updateBuzzers();

    for (int i = 0; i < NUM_VOICES; i++) {
        updateSongPlayer(voices[i]);
    }

    // -------- MOTOR CONTROL --------
if (anySongPlaying()) {
    ledcWrite(motorChannel, 200);  // ON
} else {
    ledcWrite(motorChannel, 0);    // OFF
}
}