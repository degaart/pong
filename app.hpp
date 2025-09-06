#pragma once

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>

struct Keystate
{
    bool up;
    bool down;
    bool left;
    bool right;
    bool space;
};

struct Entity
{
    static constexpr auto DISPLAY = 1;

    glm::vec2 pos; /* center */
    glm::vec2 size;
    glm::vec3 color;
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

    void onUpdate();
    void onRender();
};

