#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config/AppConfig.h"
#include "interfaces/IInputController.h"
#include "interfaces/IBuzzer.h"

enum class Direction { UP, DOWN, LEFT, RIGHT };
enum class GameState { TITLE, PLAYING, PAUSED, GAME_OVER };

enum class WallMode {
    SOLID,        // Collision with wall causes Game Over (Classic)
    PASS_THROUGH  // Wrap through walls and appear on the opposite side
};

enum class Difficulty {
    BASIC,
    MEDIUM,
    ADVANCE
};

enum class MenuRow : uint8_t {
    DIFFICULTY = 0,
    WALL_MODE  = 1,
    START_GAME = 2
};

struct Point {
    int8_t x;
    int8_t y;
    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
};

class SnakeGame {
public:
    SnakeGame(TFT_eSPI& tft, IInputController& input, IBuzzer& buzzer) 
        : _tft(tft), _input(input), _buzzer(&buzzer), _state(GameState::TITLE),
          _dir(Direction::RIGHT), _nextDir(Direction::RIGHT),
          _snakeLength(3), _score(0), _highScore(0), _level(1), _foodEaten(0),
          _speedMs(Config::Game::DIFFICULTY_BASIC.initialSpeedMs), _lastMoveTime(0),
          _animFrame(0), _lastAnimTime(0), _titleSnakeX(0),
          _wallMode(WallMode::SOLID), _difficulty(Difficulty::BASIC), _menuRow(MenuRow::START_GAME),
          _lastMenuNavTime(0), _lastMenuAction(ButtonAction::NONE) {}

    SnakeGame(TFT_eSPI& tft, IInputController& input) 
        : _tft(tft), _input(input), _buzzer(nullptr), _state(GameState::TITLE),
          _dir(Direction::RIGHT), _nextDir(Direction::RIGHT),
          _snakeLength(3), _score(0), _highScore(0), _level(1), _foodEaten(0),
          _speedMs(Config::Game::DIFFICULTY_BASIC.initialSpeedMs), _lastMoveTime(0),
          _animFrame(0), _lastAnimTime(0), _titleSnakeX(0),
          _wallMode(WallMode::SOLID), _difficulty(Difficulty::BASIC), _menuRow(MenuRow::START_GAME),
          _lastMenuNavTime(0), _lastMenuAction(ButtonAction::NONE) {}

    void init() {
        if (_buzzer) _buzzer->init();
        _input.init();
        _tft.init();
        _tft.setRotation(0);
        showTitleScreen();
    }

    void tick() {
        if (_buzzer) _buzzer->tick();
        handleInput();

        switch (_state) {
            case GameState::TITLE:
                updateTitleAnimation();
                break;
            case GameState::PLAYING:
                if (millis() - _lastMoveTime >= _speedMs) {
                    _lastMoveTime = millis();
                    stepGame();
                }
                break;
            case GameState::PAUSED:
                updatePauseBlink();
                break;
            case GameState::GAME_OVER:
                break;
        }
    }

private:
    TFT_eSPI& _tft;
    IInputController& _input;
    IBuzzer* _buzzer;

    GameState _state;
    Direction _dir;
    Direction _nextDir;

    Point _snake[Config::Game::MAX_LENGTH];
    uint16_t _snakeLength;
    Point _food;

    uint32_t _score;
    uint32_t _highScore;
    uint8_t  _level;
    uint8_t  _foodEaten;
    uint32_t _speedMs;
    unsigned long _lastMoveTime;

    uint8_t  _animFrame;
    unsigned long _lastAnimTime;
    int16_t  _titleSnakeX;
    bool     _blinkVisible = false;

    // Customization Settings
    WallMode   _wallMode;
    Difficulty _difficulty;
    MenuRow    _menuRow;

    unsigned long _lastMenuNavTime;
    ButtonAction  _lastMenuAction;

    Config::Game::DifficultyPreset getDifficultyPreset(Difficulty diff) const {
        switch (diff) {
            case Difficulty::BASIC:   return Config::Game::DIFFICULTY_BASIC;
            case Difficulty::MEDIUM:  return Config::Game::DIFFICULTY_MEDIUM;
            case Difficulty::ADVANCE: return Config::Game::DIFFICULTY_ADVANCE;
            default:                  return Config::Game::DIFFICULTY_BASIC;
        }
    }

    // =========================================================================
    // 1. Input & Controls
    // =========================================================================
    void handleInput() {
        ButtonAction btn = _input.getAction();

        if (_state == GameState::TITLE) {
            handleMenuInput(btn);
            return;
        }

        if (btn == ButtonAction::NONE) return;

        if (btn == ButtonAction::SELECT) {
            if (_state == GameState::PLAYING) {
                pauseGame();
                return;
            } else if (_state == GameState::PAUSED) {
                resumeGame();
                return;
            } else if (_state == GameState::GAME_OVER) {
                startNewGame();
                return;
            }
        }

        if (_state == GameState::GAME_OVER) {
            // Any direction returns to Title / Mode Selection menu
            if (btn == ButtonAction::UP || btn == ButtonAction::DOWN ||
                btn == ButtonAction::LEFT || btn == ButtonAction::RIGHT) {
                if (_buzzer) _buzzer->play(SoundEffect::BUTTON_CLICK);
                showTitleScreen();
                return;
            }
        }

        if (_state == GameState::PLAYING) {
            if (btn == ButtonAction::UP    && isValidTurn(_dir, Direction::UP))    _nextDir = Direction::UP;
            if (btn == ButtonAction::DOWN  && isValidTurn(_dir, Direction::DOWN))  _nextDir = Direction::DOWN;
            if (btn == ButtonAction::LEFT  && isValidTurn(_dir, Direction::LEFT))  _nextDir = Direction::LEFT;
            if (btn == ButtonAction::RIGHT && isValidTurn(_dir, Direction::RIGHT)) _nextDir = Direction::RIGHT;
        }
    }

    void handleMenuInput(ButtonAction btn) {
        unsigned long now = millis();
        if (btn == ButtonAction::NONE) {
            _lastMenuAction = ButtonAction::NONE;
            return;
        }

        // Rate-limit direction inputs so holding the stick steps smoothly
        if (btn != ButtonAction::SELECT) {
            if (btn == _lastMenuAction && (now - _lastMenuNavTime < 240)) {
                return;
            }
        }
        _lastMenuAction = btn;
        _lastMenuNavTime = now;

        if (btn == ButtonAction::UP) {
            _menuRow = static_cast<MenuRow>((static_cast<uint8_t>(_menuRow) + 2) % 3);
            if (_buzzer) _buzzer->play(SoundEffect::BUTTON_CLICK);
            drawMenuCards();
        } else if (btn == ButtonAction::DOWN) {
            _menuRow = static_cast<MenuRow>((static_cast<uint8_t>(_menuRow) + 1) % 3);
            if (_buzzer) _buzzer->play(SoundEffect::BUTTON_CLICK);
            drawMenuCards();
        } else if (btn == ButtonAction::LEFT) {
            if (_menuRow == MenuRow::DIFFICULTY) {
                _difficulty = (_difficulty == Difficulty::BASIC) ? Difficulty::ADVANCE :
                              static_cast<Difficulty>(static_cast<uint8_t>(_difficulty) - 1);
                if (_buzzer) _buzzer->play(SoundEffect::BUTTON_CLICK);
                drawMenuCards();
            } else if (_menuRow == MenuRow::WALL_MODE) {
                _wallMode = (_wallMode == WallMode::SOLID) ? WallMode::PASS_THROUGH : WallMode::SOLID;
                if (_buzzer) _buzzer->play(SoundEffect::BUTTON_CLICK);
                drawMenuCards();
            }
        } else if (btn == ButtonAction::RIGHT) {
            if (_menuRow == MenuRow::DIFFICULTY) {
                _difficulty = (_difficulty == Difficulty::ADVANCE) ? Difficulty::BASIC :
                              static_cast<Difficulty>(static_cast<uint8_t>(_difficulty) + 1);
                if (_buzzer) _buzzer->play(SoundEffect::BUTTON_CLICK);
                drawMenuCards();
            } else if (_menuRow == MenuRow::WALL_MODE) {
                _wallMode = (_wallMode == WallMode::SOLID) ? WallMode::PASS_THROUGH : WallMode::SOLID;
                if (_buzzer) _buzzer->play(SoundEffect::BUTTON_CLICK);
                drawMenuCards();
            }
        } else if (btn == ButtonAction::SELECT) {
            if (_menuRow == MenuRow::START_GAME) {
                startNewGame();
            } else if (_menuRow == MenuRow::DIFFICULTY) {
                _difficulty = (_difficulty == Difficulty::ADVANCE) ? Difficulty::BASIC :
                              static_cast<Difficulty>(static_cast<uint8_t>(_difficulty) + 1);
                if (_buzzer) _buzzer->play(SoundEffect::BUTTON_CLICK);
                drawMenuCards();
            } else if (_menuRow == MenuRow::WALL_MODE) {
                _wallMode = (_wallMode == WallMode::SOLID) ? WallMode::PASS_THROUGH : WallMode::SOLID;
                if (_buzzer) _buzzer->play(SoundEffect::BUTTON_CLICK);
                drawMenuCards();
            }
        }
    }

    bool isValidTurn(Direction current, Direction target) const {
        if (current == Direction::UP    && target == Direction::DOWN)  return false;
        if (current == Direction::DOWN  && target == Direction::UP)    return false;
        if (current == Direction::LEFT  && target == Direction::RIGHT) return false;
        if (current == Direction::RIGHT && target == Direction::LEFT)  return false;
        return true;
    }

    // =========================================================================
    // 2. Game Progression
    // =========================================================================
    void startNewGame() {
        if (_buzzer) _buzzer->play(SoundEffect::START_GAME);

        _snakeLength = 4;
        _snake[0] = { 6, 6 };
        _snake[1] = { 5, 6 };
        _snake[2] = { 4, 6 };
        _snake[3] = { 3, 6 };

        _dir = Direction::RIGHT;
        _nextDir = Direction::RIGHT;
        _score = 0;
        _level = 1;
        _foodEaten = 0;

        Config::Game::DifficultyPreset preset = getDifficultyPreset(_difficulty);
        _speedMs = preset.initialSpeedMs;
        _state = GameState::PLAYING;

        // Clear screen and redraw playfield
        _tft.fillScreen(Config::Colors::GRID_BG);
        drawHUD();
        drawPlayfieldBorder();
        spawnFood();

        for (uint16_t i = 0; i < _snakeLength; ++i) {
            drawCell(_snake[i].x, _snake[i].y, (i == 0) ? Config::Colors::SNAKE_HEAD : Config::Colors::SNAKE_BODY);
        }
    }

    void pauseGame() {
        _state = GameState::PAUSED;
        _blinkVisible = true;
        if (_buzzer) _buzzer->play(SoundEffect::PAUSE);

        // Large Arcade Modal Card
        _tft.fillRoundRect(20, 65, 200, 110, 10, Config::Colors::MODAL_BG);
        _tft.drawRoundRect(20, 65, 200, 110, 10, Config::Colors::MODAL_EDGE);
        _tft.drawRoundRect(22, 67, 196, 106, 8, Config::Colors::MODAL_EDGE);

        _tft.setTextDatum(MC_DATUM);
        _tft.setTextColor(Config::Colors::GOLD_YELLOW, Config::Colors::MODAL_BG);
        _tft.drawString("PAUSED", 120, 100, 4);

        _tft.setTextColor(Config::Colors::TEXT_WHITE, Config::Colors::MODAL_BG);
        _tft.drawString("SELECT TO RESUME", 120, 142, 2);
    }

    void resumeGame() {
        _state = GameState::PLAYING;
        if (_buzzer) _buzzer->play(SoundEffect::BUTTON_CLICK);

        _tft.fillScreen(Config::Colors::GRID_BG);
        drawHUD();
        drawPlayfieldBorder();

        drawFoodCell(_food.x, _food.y);
        for (uint16_t i = 0; i < _snakeLength; ++i) {
            drawCell(_snake[i].x, _snake[i].y, (i == 0) ? Config::Colors::SNAKE_HEAD : Config::Colors::SNAKE_BODY);
        }
    }

    void stepGame() {
        _dir = _nextDir;
        Point newHead = calculateNextHead(_snake[0], _dir);

        if (_wallMode == WallMode::PASS_THROUGH) {
            wrapPosition(newHead);
        } else {
            if (checkWallCollision(newHead)) {
                triggerGameOver();
                return;
            }
        }

        if (checkSelfCollision(newHead)) {
            triggerGameOver();
            return;
        }

        bool ateFood = checkFoodCollision(newHead);
        Point oldTail = _snake[_snakeLength - 1];

        advanceSnakeBody(newHead, ateFood);
        renderSnakeMovement(newHead, oldTail, ateFood);

        if (ateFood) {
            handleFoodEaten();
            spawnFood();
        }
    }

    Point calculateNextHead(const Point& currentHead, Direction dir) const {
        Point next = currentHead;
        switch (dir) {
            case Direction::UP:    next.y--; break;
            case Direction::DOWN:  next.y++; break;
            case Direction::LEFT:  next.x--; break;
            case Direction::RIGHT: next.x++; break;
        }
        return next;
    }

    void wrapPosition(Point& pt) const {
        if (pt.x < 0) pt.x = Config::Game::GRID_W - 1;
        else if (pt.x >= Config::Game::GRID_W) pt.x = 0;

        if (pt.y < 0) pt.y = Config::Game::GRID_H - 1;
        else if (pt.y >= Config::Game::GRID_H) pt.y = 0;
    }

    bool checkWallCollision(const Point& head) const {
        return (head.x < 0 || head.x >= Config::Game::GRID_W ||
                head.y < 0 || head.y >= Config::Game::GRID_H);
    }

    bool checkSelfCollision(const Point& head) const {
        for (uint16_t i = 0; i < _snakeLength; ++i) {
            if (_snake[i] == head) return true;
        }
        return false;
    }

    bool checkFoodCollision(const Point& head) const {
        return (head == _food);
    }

    void advanceSnakeBody(const Point& newHead, bool grow) {
        if (grow && _snakeLength < Config::Game::MAX_LENGTH) {
            _snakeLength++;
        }
        for (int16_t i = _snakeLength - 1; i > 0; --i) {
            _snake[i] = _snake[i - 1];
        }
        _snake[0] = newHead;
    }

    void handleFoodEaten() {
        Config::Game::DifficultyPreset preset = getDifficultyPreset(_difficulty);
        _score += (10 * _level * preset.scoreMultiplier);
        if (_score > _highScore) _highScore = _score;

        _foodEaten++;
        if (_foodEaten >= preset.foodPerLevel) {
            _foodEaten = 0;
            _level++;
            if (_speedMs > preset.minSpeedMs) {
                if (_speedMs >= preset.minSpeedMs + preset.speedStepMs) {
                    _speedMs -= preset.speedStepMs;
                } else {
                    _speedMs = preset.minSpeedMs;
                }
            }
            if (_buzzer) _buzzer->play(SoundEffect::LEVEL_UP);
            triggerLevelUpBanner();
        } else {
            if (_buzzer) _buzzer->play(SoundEffect::FOOD_EATEN);
        }
        drawHUD();
    }

    void spawnFood() {
        bool onSnake = true;
        while (onSnake) {
            _food.x = random(0, Config::Game::GRID_W);
            _food.y = random(0, Config::Game::GRID_H);
            onSnake = false;
            for (uint16_t i = 0; i < _snakeLength; ++i) {
                if (_snake[i] == _food) {
                    onSnake = true;
                    break;
                }
            }
        }
        drawFoodCell(_food.x, _food.y);
    }

    // =========================================================================
    // 3. Premium Arcade Typography & Interactive Menu Screens
    // =========================================================================
    void showTitleScreen() {
        _state = GameState::TITLE;
        _tft.fillScreen(Config::Colors::BG);
        _titleSnakeX = -40;

        // Outer Double Neon Border
        _tft.drawRect(3, 3, 234, 234, Config::Colors::ACCENT_CYAN);
        _tft.drawRect(5, 5, 230, 230, Config::Colors::ACCENT_CYAN);
        _tft.drawRect(7, 7, 226, 226, Config::Colors::GOLD_YELLOW);

        // Arcade Header Title
        _tft.setTextDatum(MC_DATUM);
        _tft.setTextColor(0x0200, Config::Colors::BG);
        _tft.drawString("SNAKE", 122, 28, 6);
        _tft.setTextColor(Config::Colors::SNAKE_HEAD, Config::Colors::BG);
        _tft.drawString("SNAKE", 120, 26, 6);

        // Subtitle badge
        _tft.setTextColor(Config::Colors::GOLD_YELLOW, Config::Colors::BG);
        _tft.drawString("ARCADE PRO 2040", 120, 48, 2);

        // Top Score badge
        _tft.setTextColor(Config::Colors::ACCENT_CYAN, Config::Colors::BG);
        _tft.drawString("TOP SCORE  " + String(_highScore), 120, 64, 2);

        // Draw Interactive Menu Cards
        drawMenuCards();

        // Bottom Controls Banner
        _tft.fillRect(10, 214, 220, 20, Config::Colors::HEADER_BG);
        _tft.setTextColor(Config::Colors::TEXT_WHITE, Config::Colors::HEADER_BG);
        _tft.drawString("STICK: PICK | SELECT: GO", 120, 224, 2);
    }

    void drawMenuCards() {
        drawDifficultyCard(_menuRow == MenuRow::DIFFICULTY);
        drawWallModeCard(_menuRow == MenuRow::WALL_MODE);
        drawStartButton(_menuRow == MenuRow::START_GAME);
    }

    void drawDifficultyCard(bool selected) {
        int16_t x = 12, y = 78, w = 216, h = 36;
        uint32_t bg = selected ? Config::Colors::CARD_ACTIVE : Config::Colors::CARD_IDLE;
        uint32_t border = selected ? Config::Colors::GOLD_YELLOW : 0x2965;

        _tft.fillRoundRect(x, y, w, h, 6, bg);
        _tft.drawRoundRect(x, y, w, h, 6, border);
        if (selected) _tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 5, border);

        _tft.setTextDatum(ML_DATUM);
        _tft.setTextColor(selected ? Config::Colors::GOLD_YELLOW : Config::Colors::TEXT_WHITE, bg);
        _tft.drawString(selected ? "> LEVEL" : "  LEVEL", x + 8, y + (h / 2), 2);

        _tft.setTextDatum(MR_DATUM);
        const char* name = "BASIC";
        uint32_t valColor = Config::Colors::SNAKE_HEAD;
        if (_difficulty == Difficulty::MEDIUM) {
            name = "MEDIUM";
            valColor = Config::Colors::GOLD_YELLOW;
        } else if (_difficulty == Difficulty::ADVANCE) {
            name = "ADVANCE";
            valColor = Config::Colors::FOOD_APPLE;
        }

        String label = "< " + String(name) + " >";
        _tft.setTextColor(valColor, bg);
        _tft.drawString(label, x + w - 8, y + (h / 2), 2);
    }

    void drawWallModeCard(bool selected) {
        int16_t x = 12, y = 118, w = 216, h = 36;
        uint32_t bg = selected ? Config::Colors::CARD_ACTIVE : Config::Colors::CARD_IDLE;
        uint32_t border = selected ? Config::Colors::GOLD_YELLOW : 0x2965;

        _tft.fillRoundRect(x, y, w, h, 6, bg);
        _tft.drawRoundRect(x, y, w, h, 6, border);
        if (selected) _tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 5, border);

        _tft.setTextDatum(ML_DATUM);
        _tft.setTextColor(selected ? Config::Colors::GOLD_YELLOW : Config::Colors::TEXT_WHITE, bg);
        _tft.drawString(selected ? "> WALLS" : "  WALLS", x + 8, y + (h / 2), 2);

        _tft.setTextDatum(MR_DATUM);
        bool isSolid = (_wallMode == WallMode::SOLID);
        uint32_t valColor = isSolid ? Config::Colors::DANGER_EDGE : Config::Colors::PORTAL_CYAN;
        String label = isSolid ? "< SOLID >" : "< PASS-THRU >";
        _tft.setTextColor(valColor, bg);
        _tft.drawString(label, x + w - 8, y + (h / 2), 2);
    }

    void drawStartButton(bool selected) {
        int16_t x = 12, y = 158, w = 216, h = 38;
        _tft.setTextDatum(MC_DATUM);

        if (selected) {
            _tft.fillRoundRect(x, y, w, h, 6, Config::Colors::ACCENT_CYAN);
            _tft.drawRoundRect(x, y, w, h, 6, 0xFFFF);
            _tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 5, 0xFFFF);
            _tft.setTextColor(TFT_BLACK, Config::Colors::ACCENT_CYAN);
            _tft.drawString(">> START GAME <<", x + (w / 2), y + (h / 2), 4);
        } else {
            _tft.fillRoundRect(x, y, w, h, 6, Config::Colors::CARD_IDLE);
            _tft.drawRoundRect(x, y, w, h, 6, Config::Colors::ACCENT_CYAN);
            _tft.setTextColor(Config::Colors::TEXT_WHITE, Config::Colors::CARD_IDLE);
            _tft.drawString("START GAME", x + (w / 2), y + (h / 2), 4);
        }
    }

    void updateTitleAnimation() {
        if (millis() - _lastAnimTime < 130) return;
        _lastAnimTime = millis();
        _animFrame++;

        // Pulse START button when currently highlighted
        if (_menuRow == MenuRow::START_GAME && (_animFrame % 4 == 0)) {
            drawStartButton(true);
        }

        // Animated neon snake crawl above bottom banner
        int16_t y = 202;
        _tft.fillRect(10, y, 220, 8, Config::Colors::BG);
        _titleSnakeX += 5;
        if (_titleSnakeX > 210) _titleSnakeX = -40;

        _tft.fillRoundRect(_titleSnakeX + 24, y, 10, 8, 2, Config::Colors::SNAKE_HEAD);
        _tft.fillRoundRect(_titleSnakeX + 14, y, 9,  8, 2, Config::Colors::SNAKE_BODY);
        _tft.fillRoundRect(_titleSnakeX + 5,  y, 8,  8, 2, Config::Colors::SNAKE_BODY);
    }

    void updatePauseBlink() {
        if (millis() - _lastAnimTime < 450) return;
        _lastAnimTime = millis();
        _blinkVisible = !_blinkVisible;

        _tft.setTextDatum(MC_DATUM);
        if (_blinkVisible) {
            _tft.setTextColor(Config::Colors::GOLD_YELLOW, Config::Colors::MODAL_BG);
            _tft.drawString("PAUSED", 120, 100, 4);
        } else {
            _tft.fillRect(30, 85, 180, 30, Config::Colors::MODAL_BG);
        }
    }

    void triggerLevelUpBanner() {
        _tft.fillRoundRect(30, 80, 180, 60, 8, Config::Colors::GOLD_YELLOW);
        _tft.setTextColor(TFT_BLACK, Config::Colors::GOLD_YELLOW);
        _tft.setTextDatum(MC_DATUM);
        _tft.drawString("LEVEL UP!", 120, 100, 4);
        _tft.drawString("SPEED INCREASED", 120, 125, 2);
        delay(400);

        _tft.fillScreen(Config::Colors::GRID_BG);
        drawHUD();
        drawPlayfieldBorder();
        drawFoodCell(_food.x, _food.y);
        for (uint16_t i = 0; i < _snakeLength; ++i) {
            drawCell(_snake[i].x, _snake[i].y, (i == 0) ? Config::Colors::SNAKE_HEAD : Config::Colors::SNAKE_BODY);
        }
    }

    void triggerGameOver() {
        _state = GameState::GAME_OVER;
        if (_buzzer) _buzzer->play(SoundEffect::GAME_OVER);

        // Impact flash
        _tft.drawRect(0, 0, Config::Game::SCREEN_W, Config::Game::SCREEN_H, Config::Colors::DANGER_EDGE);
        _tft.drawRect(1, 1, Config::Game::SCREEN_W - 2, Config::Game::SCREEN_H - 2, Config::Colors::DANGER_EDGE);
        delay(120);

        // Premium Game Over Modal Card
        _tft.fillRoundRect(12, 28, 216, 184, 10, Config::Colors::MODAL_BG);
        _tft.drawRoundRect(12, 28, 216, 184, 10, Config::Colors::DANGER_EDGE);
        _tft.drawRoundRect(14, 30, 212, 180, 8, Config::Colors::DANGER_EDGE);

        _tft.setTextDatum(MC_DATUM);
        _tft.setTextColor(Config::Colors::DANGER_EDGE, Config::Colors::MODAL_BG);
        _tft.drawString("GAME OVER", 120, 50, 4);

        // Score summary box
        _tft.fillRoundRect(22, 70, 196, 68, 6, 0x0000);
        _tft.drawRoundRect(22, 70, 196, 68, 6, 0x2104);

        _tft.setTextColor(Config::Colors::TEXT_WHITE, 0x0000);
        _tft.drawString("SCORE: " + String(_score), 120, 86, 4);

        const char* dName = (_difficulty == Difficulty::BASIC) ? "BASIC" :
                            (_difficulty == Difficulty::MEDIUM ? "MEDIUM" : "ADVANCE");
        const char* wName = (_wallMode == WallMode::SOLID) ? "SOLID WALL" : "PASS-THRU";

        _tft.setTextColor(Config::Colors::GOLD_YELLOW, 0x0000);
        _tft.drawString(String(dName) + " | " + String(wName), 120, 110, 2);
        _tft.setTextColor(Config::Colors::ACCENT_CYAN, 0x0000);
        _tft.drawString("BEST: " + String(_highScore), 120, 126, 2);

        // Navigation instructions
        _tft.setTextColor(Config::Colors::ACCENT_CYAN, Config::Colors::MODAL_BG);
        _tft.drawString("SELECT: PLAY AGAIN", 120, 156, 2);
        _tft.setTextColor(Config::Colors::TEXT_WHITE, Config::Colors::MODAL_BG);
        _tft.drawString("STICK: RETURN TO MENU", 120, 180, 2);
    }

    // =========================================================================
    // 4. In-Game HUD & Graphics
    // =========================================================================
    void drawPlayfieldBorder() {
        using namespace Config::Game;
        int16_t bx = OFFSET_X - 1;
        int16_t by = OFFSET_Y - 1;
        int16_t bw = (GRID_W * CELL_SIZE) + 2;
        int16_t bh = (GRID_H * CELL_SIZE) + 2;

        if (_wallMode == WallMode::SOLID) {
            // Solid glowing boundary (collision = fatal)
            _tft.drawRect(bx, by, bw, bh, Config::Colors::ACCENT_CYAN);
            _tft.drawRect(bx - 1, by - 1, bw + 2, bh + 2, Config::Colors::HEADER_LINE);
        } else {
            // Portal corner brackets indicating open pass-through edges
            constexpr int16_t L = 8;
            uint32_t col = Config::Colors::PORTAL_CYAN;
            // Top-Left
            _tft.drawFastHLine(bx, by, L, col);
            _tft.drawFastVLine(bx, by, L, col);
            // Top-Right
            _tft.drawFastHLine(bx + bw - L, by, L, col);
            _tft.drawFastVLine(bx + bw - 1, by, L, col);
            // Bottom-Left
            _tft.drawFastHLine(bx, by + bh - 1, L, col);
            _tft.drawFastVLine(bx, by + bh - L, L, col);
            // Bottom-Right
            _tft.drawFastHLine(bx + bw - L, by + bh - 1, L, col);
            _tft.drawFastVLine(bx + bw - 1, by + bh - L, L, col);
        }   }

    void drawHUD() {
        using namespace Config::Game;
        _tft.fillRect(0, 0, SCREEN_W, HEADER_H, Config::Colors::HEADER_BG);
        _tft.drawFastHLine(0, HEADER_H - 2, SCREEN_W, Config::Colors::HEADER_LINE);
        _tft.drawFastHLine(0, HEADER_H - 1, SCREEN_W, TFT_BLACK);

        // Score on left
        _tft.setTextDatum(ML_DATUM);
        _tft.setTextColor(Config::Colors::TEXT_WHITE, Config::Colors::HEADER_BG);
        _tft.drawString("SC:" + String(_score), 4, HEADER_H / 2, 2);

        // Center Level & Mode Tags
        _tft.setTextDatum(MC_DATUM);
        const char* dTag = (_difficulty == Difficulty::BASIC) ? "BAS" :
                           (_difficulty == Difficulty::MEDIUM ? "MED" : "ADV");
        const char* wTag = (_wallMode == WallMode::SOLID) ? "SOLID" : "WRAP";
        _tft.setTextColor(Config::Colors::GOLD_YELLOW, Config::Colors::HEADER_BG);
        _tft.drawString("LV" + String(_level) + " " + String(dTag) + "|" + String(wTag), 120, HEADER_H / 2, 2);

        // High Score on right
        _tft.setTextDatum(MR_DATUM);
        _tft.setTextColor(Config::Colors::ACCENT_CYAN, Config::Colors::HEADER_BG);
        _tft.drawString("HI:" + String(_highScore), SCREEN_W - 4, HEADER_H / 2, 2);
    }

    void renderSnakeMovement(const Point& newHead, const Point& oldTail, bool grew) {
        // Draw body at previous head position
        drawCell(_snake[1].x, _snake[1].y, Config::Colors::SNAKE_BODY);
        // Draw shiny new head
        drawCell(newHead.x, newHead.y, Config::Colors::SNAKE_HEAD);
        // Erase old tail
        if (!grew) {
            drawEmptyCell(oldTail.x, oldTail.y);
        }
    }

    void drawCell(int8_t gx, int8_t gy, uint32_t color) {
        using namespace Config::Game;
        int16_t px = OFFSET_X + (gx * CELL_SIZE);
        int16_t py = OFFSET_Y + (gy * CELL_SIZE);

        _tft.fillRoundRect(px + 1, py + 1, CELL_SIZE - 2, CELL_SIZE - 2, 3, color);
        _tft.drawFastHLine(px + 2, py + 2, CELL_SIZE - 5, 0xFFFF);
    }

    void drawEmptyCell(int8_t gx, int8_t gy) {
        using namespace Config::Game;
        int16_t px = OFFSET_X + (gx * CELL_SIZE);
        int16_t py = OFFSET_Y + (gy * CELL_SIZE);
        _tft.fillRect(px, py, CELL_SIZE, CELL_SIZE, Config::Colors::GRID_BG);
    }

    void drawFoodCell(int8_t gx, int8_t gy) {
        using namespace Config::Game;
        int16_t px = OFFSET_X + (gx * CELL_SIZE);
        int16_t py = OFFSET_Y + (gy * CELL_SIZE);

        _tft.fillRect(px, py, CELL_SIZE, CELL_SIZE, Config::Colors::GRID_BG);

        // Red apple body (circular)
        int16_t cx = px + (CELL_SIZE / 2);
        int16_t cy = py + (CELL_SIZE / 2);
        _tft.fillCircle(cx, cy + 1, (CELL_SIZE / 2) - 2, Config::Colors::FOOD_APPLE);
        _tft.drawPixel(cx - 1, cy - 1, 0xFFFF);
        _tft.drawFastVLine(cx, py + 1, 2, Config::Colors::FOOD_STEM);
    }
};
