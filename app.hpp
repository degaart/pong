#pragma once

#include "rng.hpp"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <optional>
#include <string>

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
    static constexpr auto SCREEN_WIDTH = 960;
    static constexpr auto SCREEN_HEIGHT = 540;
    static constexpr auto FPS = 60;
    static constexpr auto dT = 1.0f/FPS;

    App();
    SDL_AppResult onInit(int argc, char** argv);
    SDL_AppResult onEvent(SDL_Event* event);
    SDL_AppResult onIterate();
    void onQuit(SDL_AppResult result);

  private:
    SDL_Window* _window;
    SDL_Renderer* _renderer;
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

    void onUpdate();
    void onRender();
};

