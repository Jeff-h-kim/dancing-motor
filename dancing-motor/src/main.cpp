#include <Arduino.h>
#include <ArduinoJson.h>
#include "notes.h"
#include "music_files/test.h"

#define BUZZER_PIN 32

const int channel = 0;
const int resolution = 8;


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

    if (type == "reg") {
        length = 0.70;
        gap = 0.30;
    } else if (type == "stac") {
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

    tone(BUZZER_PIN, freq, playTime);
    delay(playTime + gapTime);

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

  ledcSetup(channel, 2000, resolution);
  ledcAttachPin(BUZZER_PIN, channel);

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
    playSong(music1, interval);
}

void loop() {
  // put your main code here, to run repeatedly:
};

