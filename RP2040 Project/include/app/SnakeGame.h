#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config/AppConfig.h"
#include "interfaces/IInputController.h"

enum class Direction { UP, DOWN, LEFT, RIGHT };
enum class GameState { TITLE, PLAYING, PAUSED, GAME_OVER };

struct Point {
    int8_t x;
    int8_t y;
    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
};

class SnakeGame {
public:
    SnakeGame(TFT_eSPI& tft, IInputController& input) 
        : _tft(tft), _input(input), _state(GameState::TITLE),
          _dir(Direction::RIGHT), _nextDir(Direction::RIGHT),
          _snakeLength(3), _score(0), _highScore(0), _level(1), _foodEaten(0),
          _speedMs(Config::Game::INITIAL_SPEED_MS), _lastMoveTime(0),
          _animFrame(0), _lastAnimTime(0), _titleSnakeX(0) {}

    void init() {
        _input.init();
        _tft.init();
        _tft.setRotation(0);
        showTitleScreen();
    }

    void tick() {
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

    // =========================================================================
    // 1. Input & Controls
    // =========================================================================
    void handleInput() {
        ButtonAction btn = _input.getAction();
        if (btn == ButtonAction::NONE) return;

        if (btn == ButtonAction::SELECT) {
            if (_state == GameState::TITLE || _state == GameState::GAME_OVER) {
                startNewGame();
                return;
            } else if (_state == GameState::PLAYING) {
                pauseGame();
                return;
            } else if (_state == GameState::PAUSED) {
                resumeGame();
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
        _speedMs = Config::Game::INITIAL_SPEED_MS;
        _state = GameState::PLAYING;

        // Clear screen and redraw playfield
        _tft.fillScreen(Config::Colors::GRID_BG);
        drawHUD();
        spawnFood();

        for (uint16_t i = 0; i < _snakeLength; ++i) {
            drawCell(_snake[i].x, _snake[i].y, (i == 0) ? Config::Colors::SNAKE_HEAD : Config::Colors::SNAKE_BODY);
        }
    }

    void pauseGame() {
        _state = GameState::PAUSED;
        _blinkVisible = true;

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

        _tft.fillScreen(Config::Colors::GRID_BG);
        drawHUD();

        drawFoodCell(_food.x, _food.y);
        for (uint16_t i = 0; i < _snakeLength; ++i) {
            drawCell(_snake[i].x, _snake[i].y, (i == 0) ? Config::Colors::SNAKE_HEAD : Config::Colors::SNAKE_BODY);
        }
    }

    void stepGame() {
        _dir = _nextDir;
        Point newHead = calculateNextHead(_snake[0], _dir);

        if (checkWallCollision(newHead) || checkSelfCollision(newHead)) {
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
        _score += (10 * _level);
        if (_score > _highScore) _highScore = _score;

        _foodEaten++;
        if (_foodEaten >= Config::Game::FOOD_PER_LEVEL) {
            _foodEaten = 0;
            _level++;
            if (_speedMs > Config::Game::MIN_SPEED_MS) {
                _speedMs -= Config::Game::SPEED_STEP_MS;
            }
            triggerLevelUpBanner();
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
    // 3. Premium Arcade Typography & UI Screens
    // =========================================================================
    void showTitleScreen() {
        _state = GameState::TITLE;
        _tft.fillScreen(Config::Colors::BG);
        _titleSnakeX = -40;

        // Outer Double Neon Border
        _tft.drawRect(3, 3, 234, 234, Config::Colors::ACCENT_CYAN);
        _tft.drawRect(5, 5, 230, 230, Config::Colors::ACCENT_CYAN);
        _tft.drawRect(8, 8, 224, 224, Config::Colors::GOLD_YELLOW);

        // Arcade Header Title (Using crisp font 6 + shadow)
        _tft.setTextDatum(MC_DATUM);

        // Title drop shadow
        _tft.setTextColor(0x0200, Config::Colors::BG);
        _tft.drawString("SNAKE", 122, 54, 6);
        _tft.setTextColor(Config::Colors::SNAKE_HEAD, Config::Colors::BG);
        _tft.drawString("SNAKE", 120, 52, 6);

        // Subtitle badge
        _tft.fillRoundRect(35, 82, 170, 24, 6, Config::Colors::HEADER_BG);
        _tft.drawRoundRect(35, 82, 170, 24, 6, Config::Colors::ACCENT_CYAN);
        _tft.setTextColor(Config::Colors::GOLD_YELLOW, Config::Colors::HEADER_BG);
        _tft.drawString("ARCADE PRO 2040", 120, 94, 2);

        // High Score Badge
        _tft.fillRoundRect(40, 115, 160, 26, 4, 0x0002);
        _tft.drawRoundRect(40, 115, 160, 26, 4, Config::Colors::GOLD_YELLOW);
        _tft.setTextColor(Config::Colors::TEXT_WHITE, 0x0002);
        _tft.drawString("TOP SCORE  " + String(_highScore), 120, 128, 2);

        // Bottom Controls Banner
        _tft.fillRect(10, 208, 220, 20, Config::Colors::HEADER_BG);
        _tft.setTextColor(Config::Colors::TEXT_WHITE, Config::Colors::HEADER_BG);
        _tft.drawString("D-PAD: MOVE | SEL: PAUSE", 120, 218, 2);
    }

    void updateTitleAnimation() {
        if (millis() - _lastAnimTime < 130) return;
        _lastAnimTime = millis();
        _animFrame++;

        // Big, prominent flashing CTA: Font 4
        _tft.setTextDatum(MC_DATUM);
        if (_animFrame % 4 < 2) {
            _tft.fillRoundRect(22, 150, 196, 38, 6, Config::Colors::ACCENT_CYAN);
            _tft.setTextColor(TFT_BLACK, Config::Colors::ACCENT_CYAN);
            _tft.drawString("PRESS SELECT", 120, 169, 4);
        } else {
            _tft.fillRoundRect(22, 150, 196, 38, 6, Config::Colors::BG);
            _tft.drawRoundRect(22, 150, 196, 38, 6, Config::Colors::ACCENT_CYAN);
            _tft.setTextColor(Config::Colors::GOLD_YELLOW, Config::Colors::BG);
            _tft.drawString("PRESS SELECT", 120, 169, 4);
        }

        // Animated neon snake crawl at the bottom
        int16_t y = 196;
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
        // High visibility Level-Up animation
        _tft.fillRoundRect(30, 80, 180, 60, 8, Config::Colors::GOLD_YELLOW);
        _tft.setTextColor(TFT_BLACK, Config::Colors::GOLD_YELLOW);
        _tft.setTextDatum(MC_DATUM);
        _tft.drawString("LEVEL UP!", 120, 100, 4);
        _tft.drawString("SPEED INCREASED", 120, 125, 2);
        delay(400);

        _tft.fillScreen(Config::Colors::GRID_BG);
        drawHUD();
        drawFoodCell(_food.x, _food.y);
        for (uint16_t i = 0; i < _snakeLength; ++i) {
            drawCell(_snake[i].x, _snake[i].y, (i == 0) ? Config::Colors::SNAKE_HEAD : Config::Colors::SNAKE_BODY);
        }
    }

    void triggerGameOver() {
        _state = GameState::GAME_OVER;

        // Impact flash
        _tft.drawRect(0, 0, Config::Game::SCREEN_W, Config::Game::SCREEN_H, Config::Colors::DANGER_EDGE);
        _tft.drawRect(1, 1, Config::Game::SCREEN_W - 2, Config::Game::SCREEN_H - 2, Config::Colors::DANGER_EDGE);
        delay(120);

        // Premium Game Over Modal
        _tft.fillRoundRect(15, 35, 210, 170, 10, Config::Colors::MODAL_BG);
        _tft.drawRoundRect(15, 35, 210, 170, 10, Config::Colors::DANGER_EDGE);
        _tft.drawRoundRect(17, 37, 206, 166, 8, Config::Colors::DANGER_EDGE);

        _tft.setTextDatum(MC_DATUM);
        _tft.setTextColor(Config::Colors::DANGER_EDGE, Config::Colors::MODAL_BG);
        _tft.drawString("GAME OVER", 120, 65, 4);

        // Score summary box
        _tft.fillRoundRect(30, 90, 180, 65, 6, 0x0000);
        _tft.setTextColor(Config::Colors::TEXT_WHITE, 0x0000);
        _tft.drawString("SCORE: " + String(_score), 120, 108, 4);

        _tft.setTextColor(Config::Colors::ACCENT_CYAN, 0x0000);
        _tft.drawString("LVL " + String(_level) + "  |  BEST " + String(_highScore), 120, 138, 2);

        // Call to action
        _tft.setTextColor(Config::Colors::GOLD_YELLOW, Config::Colors::MODAL_BG);
        _tft.drawString("SELECT TO RESTART", 120, 182, 2);
    }

    // =========================================================================
    // 4. In-Game HUD & Graphics
    // =========================================================================
    void drawHUD() {
        using namespace Config::Game;
        // Background banner
        _tft.fillRect(0, 0, SCREEN_W, HEADER_H, Config::Colors::HEADER_BG);
        _tft.drawFastHLine(0, HEADER_H - 2, SCREEN_W, Config::Colors::HEADER_LINE);
        _tft.drawFastHLine(0, HEADER_H - 1, SCREEN_W, TFT_BLACK);

        // Large bold score: Font 4
        _tft.setTextDatum(ML_DATUM);
        _tft.setTextColor(Config::Colors::TEXT_WHITE, Config::Colors::HEADER_BG);
        _tft.drawString("SC:" + String(_score), 6, HEADER_H / 2, 4);

        // Level indicator: Font 2
        _tft.setTextDatum(MC_DATUM);
        _tft.setTextColor(Config::Colors::GOLD_YELLOW, Config::Colors::HEADER_BG);
        _tft.drawString("LV " + String(_level), 135, HEADER_H / 2, 2);

        // Best High Score: Font 2
        _tft.setTextDatum(MR_DATUM);
        _tft.setTextColor(Config::Colors::ACCENT_CYAN, Config::Colors::HEADER_BG);
        _tft.drawString("HI " + String(_highScore) + " ", SCREEN_W - 4, HEADER_H / 2, 2);
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
        // Subtle shiny highlight edge for high-end look
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

        // Clear square first
        _tft.fillRect(px, py, CELL_SIZE, CELL_SIZE, Config::Colors::GRID_BG);

        // Red apple body (circular)
        int16_t cx = px + (CELL_SIZE / 2);
        int16_t cy = py + (CELL_SIZE / 2);
        _tft.fillCircle(cx, cy + 1, (CELL_SIZE / 2) - 2, Config::Colors::FOOD_APPLE);
        // Specular reflection highlight
        _tft.drawPixel(cx - 1, cy - 1, 0xFFFF);
        // Green stem
        _tft.drawFastVLine(cx, py + 1, 2, Config::Colors::FOOD_STEM);
    }
};
