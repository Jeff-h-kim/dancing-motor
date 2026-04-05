#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <secrets.h>
#include "notes.h"

struct Channel {
  JsonArray notes;
  int index;
  unsigned long nextChange;
    unsigned long stopTime;
  int pwmChannel;
  int tempo;
};

// ===== SERVER =====
WebServer server(80);
int buzzerPins[6] = {15, 12, 14, 27, 26, 25};
int pwmChannels[6] = {0, 1, 2, 3, 4, 5};
Channel channels[6];
StaticJsonDocument<8192> docs[6];

int buttonPin = 33;
int buttonChannel = 6;

int motorPin = 19;
int motorChannel = 7;

// ===== CHANNEL STORAGE =====
String MELODY1_JSON;
String MELODY2_JSON;
String MELODY3_JSON;
String BASS1_JSON;
String BASS2_JSON;
String BASS3_JSON;

bool lastButtonState = HIGH;

// ===== HELPER: SAFE JSON BUILDER =====
String convertChannel(const char* title, int tempo, JsonArray channel) {
  String out = "{\n";
  out += "  \"title\": \"" + String(title) + "\",\n";
  out += "  \"tempo\": " + String(tempo) + ",\n";
  out += "  \"notes\": [\n";

  bool first = true;

  // --- Iterate directly over the notes in the channel ---
  for (JsonObject note : channel) {
    if (!first) out += ",\n";
    first = false;

    String n = note["note"] | "C4";
    float d  = note["duration"] | 1.0;
    String t = note["type"] | "reg";
    String accidental = note["accidental"] | "";

    // Apply accidental
    if (accidental == "sharp") n = n.substring(0,1) + "#" + n.substring(1);
    else if (accidental == "flat") n = n.substring(0,1) + "b" + n.substring(1);
    // natural = do nothing

    out += "    { \"note\": \"" + n + "\", \"duration\": " + String(d, 2) + ", \"type\": \"" + t + "\" }";
  }

  out += "\n  ]\n}";

  // DEBUG: print converted JSON after returning it, not before
  Serial.println("Converted channel JSON for " + String(title) + ":");
  Serial.println(out);

  return out;
}

// ===== CORS HEADERS =====
void sendCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, x-api-key");
}

// ===== PING =====
void handlePing() {
  sendCORS();
  server.send(200, "text/plain", "pong");
}

// ===== OPTIONS =====
void handleOptions() {
  sendCORS();
  server.send(204);
}

// ===== MAIN UPLOAD HANDLER =====
void handleUpload() {
  sendCORS();

  // --- Handle preflight ---
  if (server.method() == HTTP_OPTIONS) {
    server.send(200);
    return;
  }

  // --- API KEY CHECK ---
  String key = "";

  if (server.hasHeader("x-api-key")) key = server.header("x-api-key");
  else if (server.hasHeader("X-API-KEY")) key = server.header("X-API-KEY");
  else if (server.hasHeader("X-Api-Key")) key = server.header("X-Api-Key");

  Serial.print("Received key: ");
  Serial.println(key);

  if (key != API_KEY) {
    Serial.println("❌ Invalid API key");
    server.send(401, "text/plain", "Unauthorized");
    return;
  }

  // --- BODY CHECK ---
  if (!server.hasArg("plain")) {
    Serial.println("❌ No body received");
    server.send(400, "text/plain", "No body");
    return;
  }

  String body = server.arg("plain");

  Serial.println("📥 Received JSON:");
  Serial.println(body);

  // --- PARSE JSON (BIG BUFFER) ---
  StaticJsonDocument<16384> doc;

  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    Serial.print("❌ JSON parse failed: ");
    Serial.println(err.c_str());
    server.send(400, "text/plain", "Bad JSON");
    return;
  }

  // --- VALIDATION ---
  if (!doc.containsKey("tempo") ||
      !doc.containsKey("treble") ||
      !doc.containsKey("bass")) {
    Serial.println("❌ Missing required fields");
    server.send(400, "text/plain", "Missing fields");
    return;
  }

  int tempo = doc["tempo"];

  JsonArray treble = doc["treble"];
  JsonArray bass   = doc["bass"];

  if (treble.size() < 3 || bass.size() < 3) {
    Serial.println("❌ Need 3 channels each");
    server.send(400, "text/plain", "Need 3 channels each");
    return;
  }

  // --- EXTRACT CHANNELS ---
  JsonArray melody1Notes   = treble[0];
  JsonArray melody2Notes = treble[1];
  JsonArray melody3Notes = treble[2];

  JsonArray bass1Notes = bass[0];
  JsonArray bass2Notes = bass[1];
  JsonArray bass3Notes = bass[2];

  // --- CONVERT ---
  MELODY1_JSON   = convertChannel("Melody1", tempo, melody1Notes);
  MELODY2_JSON = convertChannel("Melody2", tempo, melody2Notes);
  MELODY3_JSON = convertChannel("Melody3", tempo, melody3Notes);
  BASS1_JSON    = convertChannel("Bass1", tempo, bass1Notes);
  BASS2_JSON    = convertChannel("Bass2", tempo, bass2Notes);
  BASS3_JSON    = convertChannel("Bass3", tempo, bass3Notes);

  Serial.println("✅ Channels parsed successfully!");

  // --- SUCCESS RESPONSE ---
  server.send(200, "text/plain", "OK");
}



int getFreq(String note) {
  StaticJsonDocument<4096> doc;
  deserializeJson(doc, NOTES_JSON);

  if (!doc.containsKey(note)) {
    Serial.print("❌ Unknown note: ");
    Serial.println(note);
    return 0;
  }

  return doc[note]["freq"];
}






void playMelody() {
  if (MELODY1_JSON == "") {
    Serial.println("❌ No melody loaded!");
    return;
  }

  Serial.println("🎵 Playing melody...");

  StaticJsonDocument<8192> doc;
  DeserializationError err = deserializeJson(doc, MELODY1_JSON);

  if (err) {
    Serial.println("❌ Melody JSON parse failed");
    return;
  }

  JsonArray notes = doc["notes"];
  int tempo = doc["tempo"] | 120;

  // duration of a quarter note in ms
  float beatMs = 60000.0 / tempo;

  for (JsonObject n : notes) {
    String note = n["note"];
    float duration = n["duration"];
    String type = n["type"] | "reg";

    int freq = getFreq(note);

    Serial.print("Playing: ");
    Serial.println(note);

    if (freq > 0) {
      ledcWriteTone(0, freq);
    } else {
      ledcWriteTone(0, 0); // rest
    }

    int playTime = beatMs * duration;

    if (type == "stac") {
      delay(playTime * 0.5);
      ledcWriteTone(0, 0);
      delay(playTime * 0.5);
    } else {
      delay(playTime);
    }

    ledcWriteTone(0, 0); // stop between notes
  }

  Serial.println("✅ Done playing");
}


void startPlayback() {
  const String jsons[6] = {
    MELODY1_JSON, MELODY2_JSON, MELODY3_JSON,
    BASS1_JSON, BASS2_JSON, BASS3_JSON
  };

  for (int i = 0; i < 6; i++) {
    deserializeJson(docs[i], jsons[i]);

    channels[i].notes = docs[i]["notes"];
    channels[i].index = 0;
    channels[i].nextChange = millis();
    channels[i].pwmChannel = pwmChannels[i];
    channels[i].tempo = docs[i]["tempo"] | 120;
  }
}

void updatePlayback() {
    unsigned long now = millis();

    for (int i = 0; i < 6; i++) {
        Channel &ch = channels[i];

        // Stop note if its stopTime has passed
        if (ch.stopTime > 0 && now >= ch.stopTime) {
            ledcWriteTone(ch.pwmChannel, 0);
            ch.stopTime = 0;  // reset
        }

        // Check if finished
        if (ch.index >= ch.notes.size()) continue;

        // Time to play next note
        if (now >= ch.nextChange) {
            JsonObject n = ch.notes[ch.index];

            String note = n["note"];
            float duration = n["duration"];  // in beats
            String type = n["type"] | "reg";

            int freq = getFreq(note);
            float beatMs = 60000.0 / ch.tempo;
            float totalMs = duration * beatMs;

            // Determine actual note play time
            float playMs;
            if (type == "stac") playMs = totalMs * 0.5;   // staccato
            else if (type == "legato") playMs = totalMs;  // legato
            else playMs = totalMs * 0.8;                  // regular 80%

            // Start note
            if (freq > 0) ledcWriteTone(ch.pwmChannel, freq);

            // Schedule stop
            ch.stopTime = now + (unsigned long)playMs;

            // Schedule next note
            ch.nextChange = now + (unsigned long)totalMs;

            Serial.print("CH ");
            Serial.print(i);
            Serial.print(" NOTE: ");
            Serial.print(note);
            Serial.print(" FREQ: ");
            Serial.print(freq);
            Serial.print(" TYPE: ");
            Serial.print(type);
            Serial.print(" PLAYMS: ");
            Serial.println(playMs);

            ch.index++;
        }
    }
}





// ===== SETUP =====
void setup() {
  Serial.begin(115200);

pinMode(33, INPUT_PULLUP);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Routes
  server.on("/ping", HTTP_GET, handlePing);
  server.on("/upload", HTTP_OPTIONS, handleOptions);
  server.on("/upload", HTTP_POST, handleUpload);

  const char* headerKeys[] = {"x-api-key", "X-API-KEY", "X-Api-Key"};
size_t headerKeysCount = sizeof(headerKeys) / sizeof(char*);
server.collectHeaders(headerKeys, headerKeysCount);

  server.begin();
  Serial.println("🚀 Server started on port 80");

for (int i = 0; i < 6; i++) {
  ledcSetup(pwmChannels[i], 2000, 8);
  ledcAttachPin(buzzerPins[i], pwmChannels[i]);
}

Serial.println("BOOT");

 pinMode(motorPin, OUTPUT);  // <-- Make sure the pin is OUTPUT
  digitalWrite(motorPin, LOW);
Serial.println("Testing motor...");
  digitalWrite(motorPin, HIGH);  // motor on
  delay(500);
  digitalWrite(motorPin, LOW);   // motor off
  Serial.println("Motor test done");

}

// ===== LOOP =====
void loop() {
  server.handleClient();

  bool currentState = digitalRead(33);

  if (currentState == LOW) {
    // Button just pressed
    Serial.println("BUTTON PRESSED!");

    // --- Motor & buzzer test (already working) ---
    digitalWrite(motorPin, HIGH);   // motor on
    delay(500);
    digitalWrite(motorPin, LOW);    // motor off

    // --- Start music playback ---
    startPlayback();   // prepare channels
    Serial.println("🎶 Starting music playback...");
  }

  lastButtonState = currentState;

  updatePlayback();  // 🔥 THIS runs constantly
}
