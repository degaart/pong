#include "app.hpp"

#include <algorithm>
#include <assert.h>
#include <fmt/format.h>

App::App()
    : _window(nullptr), _renderer(nullptr), _prevTime(0.0), _lag(0.0),
      _theta(0.0f), _fpsTimer(0.0), _frames(0), _fps(0), _shit(nullptr)
{
    memset(&_keyState, 0, sizeof(_keyState));
}

SDL_AppResult App::onInit(int argc, char** argv)
{
    auto flags = SDL_WINDOW_RESIZABLE;
    auto ret = SDL_CreateWindowAndRenderer("Pong", SCREEN_WIDTH, SCREEN_HEIGHT,
                                           flags, &_window, &_renderer);
    if (!ret)
    {
        return SDL_APP_FAILURE;
    }

    /* Separator lines */
    for (int i = 0; i < 21; i++)
    {
        auto entity = std::make_unique<Entity>();
        entity->pos = {0.0f, -0.5f + (i * 0.05f)};
        entity->size = {0.005f, 0.03f};
        entity->flags = Entity::DISPLAY;
        entity->color.r = 0.5f;
        entity->color.g = 0.5f;
        entity->color.b = 0.5f;
        _entities.push_back(std::move(entity));
    }

    /* Ball */
    auto entity = std::make_unique<Entity>();
    entity->pos = {0.0f, 0.0f};
    entity->size = {0.01f, 0.01f};
    entity->flags = Entity::DISPLAY;
    entity->color.r = 1.0f;
    entity->color.g = 1.0f;
    entity->color.b = 1.0f;
    _entities.push_back(std::move(entity));

    /* Paddles */
    entity = std::make_unique<Entity>();
    entity->pos = {-0.4f, 0.0f};
    entity->size = {0.02f, 0.1f};
    entity->flags = Entity::DISPLAY;
    entity->color.r = 1.0f;
    entity->color.g = 0.75f;
    entity->color.b = 0.5f;
    _entities.push_back(std::move(entity));

    entity = std::make_unique<Entity>();
    entity->pos = {0.4f, 0.0f};
    entity->size = {0.02f, 0.1f};
    entity->flags = Entity::DISPLAY;
    entity->color.r = 0.5f;
    entity->color.g = 0.75f;
    entity->color.b = 1.0f;
    _entities.push_back(std::move(entity));

    return SDL_APP_CONTINUE;
}

SDL_AppResult App::onEvent(SDL_Event* event)
{
    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
    {
        if (event->key.key == SDLK_ESCAPE)
        {
            return SDL_APP_SUCCESS;
        }

        if (event->key.key == SDLK_UP)
        {
            _keyState.up = event->type == SDL_EVENT_KEY_DOWN;
        }
        else if (event->key.key == SDLK_DOWN)
        {
            _keyState.down = event->type == SDL_EVENT_KEY_DOWN;
        }
        else if (event->key.key == SDLK_LEFT)
        {
            _keyState.left = event->type == SDL_EVENT_KEY_DOWN;
        }
        else if (event->key.key == SDLK_RIGHT)
        {
            _keyState.right = event->type == SDL_EVENT_KEY_DOWN;
        }
        else if (event->key.key == SDLK_SPACE)
        {
            _keyState.space = event->type == SDL_EVENT_KEY_DOWN;
        }
    }
    break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult App::onIterate()
{
    auto beginTime = SDL_GetTicks() / 1000.0;
    auto elapsed = beginTime - _prevTime;
    _lag += elapsed;

    while (_lag > dT)
    {
        onUpdate();
        _lag -= dT;
    }
    onRender();

    _frames++;
    _fpsTimer += elapsed;
    if (_fpsTimer >= 1.0)
    {
        _fps = _frames / _fpsTimer;
        _fpsTimer = 0.0;
        _frames = 0;
    }

    auto endTime = SDL_GetTicks() / 1000.0;
    if (endTime - beginTime < 1.0 / FPS)
    {
        auto delay = ((1.0 / FPS) - (endTime - beginTime)) * 1000;
        SDL_Delay(static_cast<uint32_t>(delay));
    }
    _prevTime = beginTime;

    return SDL_APP_CONTINUE;
}

void App::onQuit(SDL_AppResult result)
{
}

void App::onUpdate()
{
    float speed = 0.05f * dT;
    if (_keyState.left)
    {
        if (_shit->pos.x - speed > -0.5f)
        {
            _shit->pos.x -= speed;
        }
    }
    else if (_keyState.right)
    {
        _shit->pos.x += speed;
    }

    if (_keyState.up)
    {
        if (_shit->pos.y - speed > -0.5f)
        {
            _shit->pos.y -= speed;
        }
    }
    else if (_keyState.down)
    {
        _shit->pos.y += speed;
    }
}

void App::onRender()
{
    SDL_SetRenderDrawColor(_renderer, 0x64, 0x95, 0xED, 0xFF);
    SDL_RenderClear(_renderer);

    int screenWidth, screenHeight;
    SDL_GetRenderOutputSize(_renderer, &screenWidth, &screenHeight);

    /* GameScreen */
    SDL_FRect rcGameScreen;
    rcGameScreen.w = screenWidth > screenHeight ? screenHeight : screenWidth;
    rcGameScreen.h = rcGameScreen.w;
    rcGameScreen.x = (screenWidth - rcGameScreen.w) / 2;
    rcGameScreen.y = (screenHeight - rcGameScreen.h) / 2;

    SDL_SetRenderDrawColor(_renderer, 0x10, 0x10, 0x10, 0xFF);
    SDL_RenderFillRect(_renderer, &rcGameScreen);

    /* Entities (they are just rectangles) */
    /*
     * Coordinate system:
     * top left: 0.0,0.0
     * bottom right: 1.0,1.0
     */
    /* Trans      Scale            Trans */
    auto transformX = [rcGameScreen](float x)
    { return ((x + 0.5f) * rcGameScreen.w) + rcGameScreen.x; };
    auto transformY = [rcGameScreen](float y)
    { return ((y + 0.5f) * rcGameScreen.h) + rcGameScreen.y; };
    auto scaleX = [rcGameScreen](float w) { return w * rcGameScreen.w; };
    auto scaleY = [rcGameScreen](float h) { return h * rcGameScreen.h; };

    for (const auto& entity : _entities)
    {
        SDL_FRect rc;
        rc.x = transformX(entity->pos.x - (entity->size.x / 2.0f));
        rc.y = transformY(entity->pos.y - (entity->size.y / 2.0f));
        rc.w = scaleX(entity->size.x);
        rc.h = scaleY(entity->size.y);
        SDL_SetRenderDrawColor(_renderer, std::round(entity->color.r * 255.0f),
                               std::round(entity->color.g * 255.0f),
                               std::round(entity->color.b * 255.0f), 0xFF);
        SDL_RenderFillRect(_renderer, &rc);
    }

    /* Debug text */
    auto debugText = fmt::format("fps={} pos={:0.1}x{:0.1}",
                                 _fps,
                                 _shit ? _shit->pos.x : 0,
                                 _shit ? _shit->pos.y : 0);
    SDL_SetRenderClipRect(_renderer, nullptr);
    SDL_SetRenderDrawColor(_renderer, 255, 255, 64, 255);
    SDL_RenderDebugText(_renderer, 10.0f, 10.0f, debugText.c_str());

    SDL_RenderPresent(_renderer);
}
