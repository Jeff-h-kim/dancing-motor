#pragma once

// ============================================================
//  harmony_demo.h  —  3-voice edition (melody + harmony + bass)
//
//  Channel 0 → GPIO 32  — Melody   (E6 range)
//  Channel 1 → GPIO 26  — Harmony  (third below melody, C6 range)
//  Channel 2 → GPIO 27  — Bass     (root notes, C5/G4 range)
//
//  Update main.cpp: change NUM_VOICES to 3 and add:
//    JsonDocument harmonyDoc;
//    voices[2] = { false, 0, 2, &harmonyDoc };
//  In setup(): loadSong(HARMONY_JSON, harmonyDoc);
//  In startAllVoices(): already handles it if NUM_VOICES = 3
// ============================================================

// ----------------------------------------------------------
//  CHANNEL 0 — Melody (6th octave)
// ----------------------------------------------------------
const char *MELODY_JSON = R"({
    "title": "Ode to Joy - Melody",
    "tempo": 120,
    "notes": [
        { "note": "E6",  "duration": 1.0, "type": "reg" },
        { "note": "E6",  "duration": 1.0, "type": "reg" },
        { "note": "F6",  "duration": 1.0, "type": "reg" },
        { "note": "G6",  "duration": 1.0, "type": "reg" },

        { "note": "G6",  "duration": 1.0, "type": "reg" },
        { "note": "F6",  "duration": 1.0, "type": "reg" },
        { "note": "E6",  "duration": 1.0, "type": "reg" },
        { "note": "D6",  "duration": 1.0, "type": "reg" },

        { "note": "C6",  "duration": 1.0, "type": "reg" },
        { "note": "C6",  "duration": 1.0, "type": "reg" },
        { "note": "D6",  "duration": 1.0, "type": "reg" },
        { "note": "E6",  "duration": 1.0, "type": "reg" },

        { "note": "E6",  "duration": 1.5, "type": "lega" },
        { "note": "D6",  "duration": 0.5, "type": "reg"  },
        { "note": "D6",  "duration": 2.0, "type": "lega" },

        { "note": "E6",  "duration": 1.0, "type": "reg" },
        { "note": "E6",  "duration": 1.0, "type": "reg" },
        { "note": "F6",  "duration": 1.0, "type": "reg" },
        { "note": "G6",  "duration": 1.0, "type": "reg" },

        { "note": "G6",  "duration": 1.0, "type": "reg" },
        { "note": "F6",  "duration": 1.0, "type": "reg" },
        { "note": "E6",  "duration": 1.0, "type": "reg" },
        { "note": "D6",  "duration": 1.0, "type": "reg" },

        { "note": "C6",  "duration": 1.0, "type": "reg" },
        { "note": "C6",  "duration": 1.0, "type": "reg" },
        { "note": "D6",  "duration": 1.0, "type": "reg" },
        { "note": "E6",  "duration": 1.0, "type": "reg" },

        { "note": "D6",  "duration": 1.5, "type": "lega" },
        { "note": "C6",  "duration": 0.5, "type": "reg"  },
        { "note": "C6",  "duration": 2.0, "type": "lega" }
    ]
})";

// ----------------------------------------------------------
//  CHANNEL 1 — Harmony (diatonic third below melody, 5th octave)
//  Runs in parallel with melody — same rhythm, different pitch.
//  Both buzzers play simultaneously for a chord effect.
// ----------------------------------------------------------
const char *HARMONY_JSON = R"({
    "title": "Ode to Joy - Harmony",
    "tempo": 120,
    "notes": [
        { "note": "C6",  "duration": 1.0, "type": "reg" },
        { "note": "C6",  "duration": 1.0, "type": "reg" },
        { "note": "D6",  "duration": 1.0, "type": "reg" },
        { "note": "E6",  "duration": 1.0, "type": "reg" },

        { "note": "E6",  "duration": 1.0, "type": "reg" },
        { "note": "D6",  "duration": 1.0, "type": "reg" },
        { "note": "C6",  "duration": 1.0, "type": "reg" },
        { "note": "B5",  "duration": 1.0, "type": "reg" },

        { "note": "A5",  "duration": 1.0, "type": "reg" },
        { "note": "A5",  "duration": 1.0, "type": "reg" },
        { "note": "B5",  "duration": 1.0, "type": "reg" },
        { "note": "C6",  "duration": 1.0, "type": "reg" },

        { "note": "C6",  "duration": 1.5, "type": "lega" },
        { "note": "B5",  "duration": 0.5, "type": "reg"  },
        { "note": "B5",  "duration": 2.0, "type": "lega" },

        { "note": "C6",  "duration": 1.0, "type": "reg" },
        { "note": "C6",  "duration": 1.0, "type": "reg" },
        { "note": "D6",  "duration": 1.0, "type": "reg" },
        { "note": "E6",  "duration": 1.0, "type": "reg" },

        { "note": "E6",  "duration": 1.0, "type": "reg" },
        { "note": "D6",  "duration": 1.0, "type": "reg" },
        { "note": "C6",  "duration": 1.0, "type": "reg" },
        { "note": "B5",  "duration": 1.0, "type": "reg" },

        { "note": "A5",  "duration": 1.0, "type": "reg" },
        { "note": "A5",  "duration": 1.0, "type": "reg" },
        { "note": "B5",  "duration": 1.0, "type": "reg" },
        { "note": "C6",  "duration": 1.0, "type": "reg" },

        { "note": "B5",  "duration": 1.5, "type": "lega" },
        { "note": "A5",  "duration": 0.5, "type": "reg"  },
        { "note": "A5",  "duration": 2.0, "type": "lega" }
    ]
})";

// ----------------------------------------------------------
//  CHANNEL 2 — Bass (4th/5th octave, staccato downbeats)
//  Sits two octaves below melody so frequencies are far
//  apart — minimises beating interference on shared ground.
// ----------------------------------------------------------
const char *BASS_JSON = R"({
    "title": "Ode to Joy - Bass",
    "tempo": 120,
    "notes": [
        { "note": "C5", "duration": 2.0, "type": "stac" },
        { "note": "G4", "duration": 2.0, "type": "stac" },

        { "note": "C5", "duration": 2.0, "type": "stac" },
        { "note": "G4", "duration": 2.0, "type": "stac" },

        { "note": "F4", "duration": 2.0, "type": "stac" },
        { "note": "C5", "duration": 2.0, "type": "stac" },

        { "note": "G4", "duration": 2.0, "type": "stac" },
        { "note": "G4", "duration": 2.0, "type": "stac" },

        { "note": "C5", "duration": 2.0, "type": "stac" },
        { "note": "G4", "duration": 2.0, "type": "stac" },

        { "note": "C5", "duration": 2.0, "type": "stac" },
        { "note": "G4", "duration": 2.0, "type": "stac" },

        { "note": "F4", "duration": 2.0, "type": "stac" },
        { "note": "C5", "duration": 2.0, "type": "stac" },

        { "note": "G4", "duration": 2.0, "type": "stac" },
        { "note": "C5", "duration": 2.0, "type": "stac" }
    ]
})";