#pragma once

#include "rng.hpp"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <vector>

struct Keystate
{
    bool up;
    bool down;
    bool left;
    bool right;
    bool space;
};

struct Polygon2D
{
    bool visible;
    std::vector<glm::vec2> vertices;
    glm::vec2 pos;
    glm::vec2 velocity;
    glm::vec3 color;
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
    std::vector<Polygon2D> _polygons;
    Keystate _keyState;
    double _prevTime;
    double _lag;
    float _theta;
    double _fpsTimer;
    int _frames;
    int _fps;

    void onUpdate();
    void onRender();

    Polygon2D createPolygon(Rng& rng, int vertCount, float size);
    void drawPolygon(const Polygon2D& poly);
    Polygon2D rotatePolygon(Polygon2D poly, float theta);
};

