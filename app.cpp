#include "app.hpp"

#include "font.hpp"
#include <assert.h>
#include <fmt/format.h>

App::App()
    : _window(nullptr), _renderer(nullptr), _prevTime(0.0), _lag(0.0),
      _theta(0.0f), _fpsTimer(0.0), _frames(0), _fps(0), _scores {0, 0}

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

    /* draw functions (lambdas) */
    /*
     * Coordinate system:
     * top left: 0.0,0.0
     * bottom right: 1.0,1.0
     */
    auto drawRect = [rcGameScreen, renderer = _renderer](
                        glm::vec2 pos, glm::vec2 size, glm::vec3 col)
    {
        SDL_SetRenderDrawColor(renderer, std::round(col.r * 255.0f),
                               std::round(col.g * 255.0f),
                               std::round(col.b * 255.0f), 0xFF);

        SDL_FRect rc;
        rc.x =
            (pos.x - (size.x / 2.0f) + 0.5f) * rcGameScreen.w + rcGameScreen.x;
        rc.y =
            (pos.y - (size.y / 2.0f) + 0.5f) * rcGameScreen.h + rcGameScreen.y;
        rc.w = size.x * rcGameScreen.w;
        rc.h = size.y * rcGameScreen.h;
        SDL_RenderFillRect(renderer, &rc);
    };
    auto drawDigit = [drawRect, renderer = _renderer](char digit, glm::vec2 pos,
                                                      glm::vec3 col)
    {
        assert(digit >= '0' && digit <= '9');
        const char* charData = FONT_DATA[digit - '0'];
        for (int j = 0; j < 5; j++)
        {
            for (int i = 0; i < 3; i++)
            {
                if (charData[j * 3 + i] != ' ')
                {
                    drawRect(glm::vec2 {i * 0.02f, j * 0.02f} + pos,
                             {0.02f, 0.02f}, col);
                }
            }
        }
    };

    /* Score */
    static const float scoreLocations[] = {-0.48, 0.37f};
    for (int i = 0; i < 2; i++)
    {
        auto digit1 = (_scores[i] / 10) % 10;
        if (digit1)
        {
            drawDigit(digit1 + '0', {scoreLocations[i], -0.48f},
                      {0.5f, 0.7f, 0.0f});
        }

        auto digit2 = (_scores[i] % 10);
        drawDigit(digit2 + '0', {scoreLocations[i] + 0.07f, -0.48f},
                  {0.5f, 0.7f, 0.0f});
    }

    /* Entities (they are just rectangles) */
    for (const auto& entity : _entities)
    {
        drawRect(entity->pos, entity->size, entity->color);
    }

    /* Debug text */
    auto debugText = fmt::format("fps={}", _fps);
    SDL_SetRenderClipRect(_renderer, nullptr);
    SDL_SetRenderDrawColor(_renderer, 255, 255, 64, 255);
    SDL_RenderDebugText(_renderer, 10.0f, 10.0f, debugText.c_str());

    SDL_RenderPresent(_renderer);
}
