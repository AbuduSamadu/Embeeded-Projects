#pragma once
#include <stdint.h>

enum class SoundEffect {
    NONE,
    START_GAME,
    FOOD_EATEN,
    LEVEL_UP,
    GAME_OVER,
    BUTTON_CLICK,
    PAUSE
};

class IBuzzer {
public:
    virtual ~IBuzzer() = default;
    virtual void init() = 0;
    virtual void play(SoundEffect effect) = 0;
    virtual void tick() = 0;
    virtual void stop() = 0;
    virtual void mute(bool muted) = 0;
};
