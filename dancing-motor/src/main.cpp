#include <Arduino.h>
#include <ArduinoJson.h>
#include "notes.h"
#include "music_files/test.h"

#define BUZZER_PIN 32

JsonDocument notes;
JsonDocument music1;

bool debug = false;

//-------- Functions ----------

void playNote(String note, int duration, int gap) {
    if (!notes[note]) {
        Serial.println("Note not found: " + note);
        return;
    }

    if (notes[note] == "Rest") {
      delay(duration);
      if (debug) {
        Serial.println(note);
      }
      
    }

    int freq = notes[note]["freq"].as<int>();
    tone(BUZZER_PIN, freq, duration);
    delay(duration + gap);
    if (debug) {
      Serial.println(note);
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

void playSong (JsonDocument& sheetMusic) {
  JsonArray music_notes = sheetMusic["notes"];

  for (int i = 0; i < music_notes.size(); i++) {
    const char* pitch = music_notes[i]["note"];
    const int length = music_notes[i]["duration"];
    const int rest = music_notes[i]["gap"];
    playNote(pitch, length, rest);
  }
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial) { delay(1000); } 
  delay(1000);   
  DeserializationError note_err = deserializeJson(notes, NOTES_JSON);
    if (note_err) {
        Serial.println("JSON parse failed: " + String(note_err.c_str()));
        return;
    }
    Serial.println("Notes loaded OK");

    loadSong(MUSIC_JSON, music1);
    playSong(music1);
}

void loop() {
  // put your main code here, to run repeatedly:
};

