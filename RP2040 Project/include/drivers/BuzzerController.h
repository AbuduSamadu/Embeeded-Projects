#pragma once
#include <Arduino.h>
#include "interfaces/IBuzzer.h"
#include "config/AppConfig.h"

struct ToneNote {
    uint16_t frequency; // Frequency in Hz (0 denotes rest / silence)
    uint16_t durationMs; // Duration in milliseconds
};

namespace Melodies {
    // 8-bit retro arcade fanfare on game start (C5 -> E5 -> G5 -> C6)
    static const ToneNote START[] = {
        { 523, 60 },
        { 659, 60 },
        { 784, 60 },
        { 1046, 130 }
    };

    // Snappy two-tone bite / chirp on food consumption
    static const ToneNote FOOD[] = {
        { 1046, 35 },
        { 1318, 45 }
    };

    // Triumphant ascending arpeggio on level completion
    static const ToneNote LEVEL_UP[] = {
        { 659, 70 },
        { 784, 70 },
        { 988, 70 },
        { 1318, 160 }
    };

    // Dramatic descending sequence on collision / defeat
    static const ToneNote GAME_OVER[] = {
        { 440, 90 },
        { 349, 90 },
        { 261, 110 },
        { 174, 200 }
    };

    // Crisp pause chime
    static const ToneNote PAUSE[] = {
        { 587, 40 },
        { 0,   30 },
        { 440, 50 }
    };

    // Short UI confirmation click
    static const ToneNote CLICK[] = {
        { 880, 25 }
    };
}

class BuzzerController : public IBuzzer {
public:
    explicit BuzzerController(uint8_t pin = Config::Pins::BUZZER)
        : _pin(pin),
          _muted(false),
          _isPlaying(false),
          _currentMelody(nullptr),
          _melodyLength(0),
          _currentNoteIndex(0),
          _noteStartTime(0),
          _currentNoteDuration(0) {}

    void init() override {
        pinMode(_pin, OUTPUT);
        digitalWrite(_pin, LOW);
        noTone(_pin);
    }

    void mute(bool muted) override {
        _muted = muted;
        if (_muted && _isPlaying) {
            stop();
        }
    }

    void play(SoundEffect effect) override {
        if (!Config::BuzzerConfig::ENABLED || _muted) return;

        switch (effect) {
            case SoundEffect::START_GAME:
                startMelody(Melodies::START, sizeof(Melodies::START) / sizeof(Melodies::START[0]));
                break;
            case SoundEffect::FOOD_EATEN:
                startMelody(Melodies::FOOD, sizeof(Melodies::FOOD) / sizeof(Melodies::FOOD[0]));
                break;
            case SoundEffect::LEVEL_UP:
                startMelody(Melodies::LEVEL_UP, sizeof(Melodies::LEVEL_UP) / sizeof(Melodies::LEVEL_UP[0]));
                break;
            case SoundEffect::GAME_OVER:
                startMelody(Melodies::GAME_OVER, sizeof(Melodies::GAME_OVER) / sizeof(Melodies::GAME_OVER[0]));
                break;
            case SoundEffect::PAUSE:
                startMelody(Melodies::PAUSE, sizeof(Melodies::PAUSE) / sizeof(Melodies::PAUSE[0]));
                break;
            case SoundEffect::BUTTON_CLICK:
                startMelody(Melodies::CLICK, sizeof(Melodies::CLICK) / sizeof(Melodies::CLICK[0]));
                break;
            case SoundEffect::NONE:
            default:
                stop();
                break;
        }
    }

    void tick() override {
        if (!_isPlaying || _currentMelody == nullptr) return;

        unsigned long now = millis();
        if (now - _noteStartTime >= _currentNoteDuration) {
            _currentNoteIndex++;
            if (_currentNoteIndex < _melodyLength) {
                playCurrentNote();
            } else {
                stop();
            }
        }
    }

    void stop() override {
        _isPlaying = false;
        _currentMelody = nullptr;
        _melodyLength = 0;
        noTone(_pin);
        digitalWrite(_pin, LOW);
    }

private:
    void startMelody(const ToneNote* melody, uint8_t length) {
        _currentMelody = melody;
        _melodyLength = length;
        _currentNoteIndex = 0;
        _isPlaying = true;
        playCurrentNote();
    }

    void playCurrentNote() {
        if (_currentMelody == nullptr || _currentNoteIndex >= _melodyLength) {
            stop();
            return;
        }

        const ToneNote& note = _currentMelody[_currentNoteIndex];
        _currentNoteDuration = note.durationMs;
        _noteStartTime = millis();

        if (note.frequency > 0 && !_muted) {
            tone(_pin, note.frequency);
        } else {
            noTone(_pin);
            digitalWrite(_pin, LOW);
        }
    }

    uint8_t _pin;
    bool _muted;
    bool _isPlaying;
    const ToneNote* _currentMelody;
    uint8_t _melodyLength;
    uint8_t _currentNoteIndex;
    unsigned long _noteStartTime;
    uint16_t _currentNoteDuration;
};
