#pragma once

#include "rng.hpp"
#include "sfx.hpp"
#include <SDL3/SDL.h>
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct Keystate
{
    bool up;
    bool down;
    bool left;
    bool right;
    bool space;
};

struct Rect
{
    float x, y;
    float w, h;
};

struct Entity
{
    static constexpr auto DISPLAY = 1;
    static constexpr auto PHYSICS = 2;

    glm::vec2 pos; /* center */
    glm::vec2 size;
    glm::vec3 color;
    glm::vec3 origColor;
    glm::vec2 v;
    glm::vec2 a;
    std::optional<glm::vec2> pv; /* penetration vector */
    unsigned flags;
    std::function<void(Entity&, Entity&, glm::vec2)> onCollision;
    std::function<void(Entity&, const Keystate&)> onUpdate;
    std::string name;
};

/*
 * SDL_AppResult:
 *  - SDL_APP_FAILURE
 *  - SDL_APP_SUCCESS
 *  - SDL_APP_CONTINUE
 */
class App
{
public:
    static constexpr auto GAME_WIDTH = 1.77f; /* 16:9 screen ratio */
    static constexpr auto GAME_HEIGHT = 1.0f;
    static constexpr auto GAME_SCALE = 0.95f;
    static constexpr auto BALL_SPEED = 0.75f;
    static constexpr auto FPS = 60;
    static constexpr auto PADDLE_SPEED = BALL_SPEED*0.8f;
    static constexpr auto SCREEN_HEIGHT = 540;
    static constexpr auto SCREEN_WIDTH = 960;
    static constexpr auto dT = 1.0f / FPS;
    static constexpr auto SCORE_SIZE = 0.02f;
    static constexpr glm::vec3 COLOR_BACKGROUND = { 0.39f, 0.58f, 0.93f };
    static constexpr glm::vec3 COLOR_DEBUGTEXT = { 1.0f, 1.0f, 0.25f };
    static constexpr glm::vec3 COLOR_GAMESCREEN = { 0.04f, 0.04f, 0.04f };
    static constexpr glm::vec3 COLOR_SCORE = { 0.5f, 0.7f, 0.0f };
    static constexpr glm::vec3 COLOR_BALL = { 0.9f, 0.9f, 0.9f };

    App();
    SDL_AppResult onInit(int argc, char** argv);
    SDL_AppResult onEvent(SDL_Event* event);
    SDL_AppResult onIterate();
    void onQuit(SDL_AppResult result);

private:
    SDL_Window* _window;
    SDL_Renderer* _renderer;
    SDL_AudioStream* _audioStream;
    std::vector<std::unique_ptr<Entity>> _entities;
    Keystate _keyState;
    double _prevTime;
    double _lag;
    float _theta;
    double _fpsTimer;
    int _frames;
    int _fps;
    int _scores[2];
    Entity* _ball;
    std::string _debugText;
    Rng _rng;
    Sfx _startSound;
    Sfx _bounceSound;
    Sfx _loseSound;
    bool _idle;
    bool _vSync;

    void reset();
    void onUpdate();
    void onRender(double lag);
    void playSound(const Sfx& sound);
    static std::vector<unsigned char> loadFile(const char* filename);
};

