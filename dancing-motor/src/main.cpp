#include <Arduino.h>
#include <ArduinoJson.h>
#include "notes.h"
#include "music_files/toothless_dancing.h"


#define BUZZER_PIN 32

const int motorPin = 25;
const int motorChannel = 1;
const int motorResolution = 8;
const int motorFreq = 1000;

const int buzzerChannel = 0;
const int buzzerResolution = 8;

const int buttonPin = 33;

JsonDocument notes;
JsonDocument music1;

bool debug = false;
int interval = 500;

//-------- Functions ----------
void playNote(String note, float duration, String type, int interval) {
    if (!notes[note]) {
        Serial.println("Note not found: " + note);
        return;
    }

    float length = 0.70;
    float gap = 0.30;

    if (type == "stac") {
        length = 0.40;
        gap = 0.60;
    } else if (type == "lega") {
        length = 0.90;
        gap = 0.10;
    }

    float totalTime = duration * interval;

    // Handle rest
    if (notes[note] == "Rest") {
        delay(totalTime);
        return;
    }

    int freq = notes[note]["freq"].as<int>();
    float playTime = totalTime * length;
    float gapTime = totalTime * gap;

    // Play note using PWM
    ledcWriteTone(buzzerChannel, freq);  // start note
    delay(playTime);
    ledcWriteTone(buzzerChannel, 0);     // stop note

    delay(gapTime);

    if (debug) {
        Serial.println("Note: " + note + " | total duration: " + totalTime);
    }
}

// --- load a song from a JSON string ---
bool loadSong(const char* songJson, JsonDocument& location) {
    DeserializationError sheet_music_err = deserializeJson(location, songJson);
    if (sheet_music_err) {
        Serial.println("Failed to load song: " + String(sheet_music_err.c_str()));
        return false;
    }
    Serial.println("Song loaded: " + String(location["title"].as<const char*>()));
    return true;
}

void playSong (JsonDocument& sheetMusic, int interval) {
  JsonArray music_notes = sheetMusic["notes"];

  for (int i = 0; i < music_notes.size(); i++) {
    const char* pitch = music_notes[i]["note"];
    const float length = music_notes[i]["duration"];
    const char* type = music_notes[i]["type"];
    playNote(pitch, length, type, interval);
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);   

  ledcSetup(motorChannel, motorFreq, motorResolution);
  ledcAttachPin(motorPin, motorChannel);

  ledcSetup(buzzerChannel, 2000, buzzerResolution); // initial freq doesn't matter
  ledcAttachPin(BUZZER_PIN, buzzerChannel);

  pinMode(buttonPin, INPUT_PULLUP);

  DeserializationError note_err = deserializeJson(notes, NOTES_JSON);
    if (note_err) {
        Serial.println("JSON parse failed: " + String(note_err.c_str()));
        return;
    }
    Serial.println("Notes loaded OK");

    loadSong(MUSIC_JSON, music1);
    int tempo = music1["tempo"].as<int>();

    interval = 60000/tempo;
    Serial.println(interval);
    
    

}

void loop() {
  // put your main code here, to run repeatedly:
  if (digitalRead(buttonPin) == LOW) {  // pressed
    ledcWrite(motorChannel, 200);
    playSong(music1, interval);
    ledcWrite(motorChannel, 0);
    while(digitalRead(buttonPin) == LOW); // wait for release
  }

};

