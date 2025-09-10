#include "app.hpp"

#include "font.hpp"
#include <assert.h>
#include <chrono>
#include <fmt/format.h>
#include <optional>

/*** Utility functions ****************************************************************/
static bool isColliding(const Entity& a, const Entity& b)
{
    return
        /* left side of a is at the left of the right side of b */
        (a.pos.x - (a.size.x / 2.0f) < b.pos.x + (b.size.x / 2.0f)) &&

        /* right side of a is at the right of the left side of b */
        (a.pos.x + (a.size.x / 2.0f) > b.pos.x - (b.size.x / 2.0f)) &&

        /* top side of a is over botton side of b */
        (a.pos.y - (a.size.y / 2.0f) < b.pos.y + (b.size.y / 2.0f)) &&

        /* bottom side of a is underneath top side of b */
        (a.pos.y + (a.size.y / 2.0f) > b.pos.y - (b.size.y / 2.0f));
}

static std::optional<glm::vec2> penetrationVector(const Entity& a, const Entity& b)
{
    glm::vec2 d = b.pos - a.pos;
    float px = (a.size.x / 2.0f + b.size.x / 2.0f) - std::abs(d.x);
    float py = (a.size.y / 2.0f + b.size.y / 2.0f) - std::abs(d.y);
    if (px <= 0.0f || py <= 0.0f)
    {
        return std::nullopt;
    }

    if (px < py)
    {
        return glm::vec2 {d.x < 0.0f ? -px : px, 0.0f};
    }
    else
    {
        return glm::vec2 {0.0f, d.y < 0.0f ? -py : py};
    }
}

/*** Member functions *****************************************************************/
App::App()
    : _window(nullptr),
      _renderer(nullptr),
      _prevTime(0.0),
      _lag(0.0),
      _theta(0.0f),
      _fpsTimer(0.0),
      _frames(0),
      _fps(0),
      _scores {0, 0},
      _ball(nullptr),
      _p1(nullptr),
      _p2(nullptr)

{
    memset(&_keyState, 0, sizeof(_keyState));
    _rng.seed(std::chrono::system_clock::now().time_since_epoch().count());
}

void App::reset()
{
    _ball->pos.x = _ball->pos.y = 0.0f;
    _ball->v.x = _ball->v.y = 0.0f;
}

SDL_AppResult App::onInit(int argc, char** argv)
{
    auto flags = SDL_WINDOW_RESIZABLE;
    auto ret = SDL_CreateWindowAndRenderer("Pong", SCREEN_WIDTH, SCREEN_HEIGHT, flags, &_window, &_renderer);
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

    std::unique_ptr<Entity> entity;

    auto bounce = [](Entity& self, Entity& ball, glm::vec2 pv)
    {
        ball.pos += glm::vec2 {pv.x, pv.y};
        if (pv.x < 0.0f || pv.x > 0.0f) /* horizontal collision */
        {
            ball.v.x = -ball.v.x;
        }
        if (pv.y < 0.0f || pv.y > 0.0f) /* vertical collision */
        {
            ball.v.y = -ball.v.y;
        }
    };

    /* Left wall */
    entity = std::make_unique<Entity>();
    entity->size = {0.1f, 1.0f};
    entity->pos = {-0.5f - (entity->size.x / 2.0f), 0.0f};
    entity->flags = Entity::DISPLAY | Entity::PHYSICS;
    entity->color.r = 1.0f;
    entity->color.g = 0.5f;
    entity->color.b = 1.0f;
    entity->onCollision = [this, bounce](Entity& self, Entity& other, glm::vec2 pv)
    {
        if (&other == _ball)
        {
            _scores[1]++;
            reset();
        }
    };
    entity->name = "leftwall";
    _entities.push_back(std::move(entity));

    /* Right wall */
    entity = std::make_unique<Entity>();
    entity->size = {0.1f, 1.0f};
    entity->pos = {0.5f + (entity->size.x / 2.0f), 0.0f};
    entity->flags = Entity::DISPLAY | Entity::PHYSICS;
    entity->color.r = 1.0f;
    entity->color.g = 0.5f;
    entity->color.b = 1.0f;
    entity->onCollision = [this, bounce](Entity& self, Entity& other, glm::vec2 pv)
    {
        if (&other == _ball)
        {
            _scores[0]++;
            reset();
        }
    };
    entity->name = "rightwall";
    _entities.push_back(std::move(entity));

    /* Top wall */
    entity = std::make_unique<Entity>();
    entity->size = {1.2f, 0.1f};
    entity->pos = {0.0f, -0.5f - (entity->size.y / 2.0f)};
    entity->flags = Entity::DISPLAY | Entity::PHYSICS;
    entity->color.r = 0.5f;
    entity->color.g = 1.0f;
    entity->color.b = 1.0f;
    entity->onCollision = [this, bounce](Entity& self, Entity& other, glm::vec2 pv)
    {
        if (&other == _ball)
        {
            bounce(self, other, pv);
        }
    };
    entity->name = "topwall";
    _entities.push_back(std::move(entity));

    /* Bottom wall */
    entity = std::make_unique<Entity>();
    entity->size = {1.2f, 0.1f};
    entity->pos = {0.0f, 0.5f + (entity->size.y / 2.0f)};
    entity->flags = Entity::DISPLAY | Entity::PHYSICS;
    entity->color.r = 0.5f;
    entity->color.g = 1.0f;
    entity->color.b = 1.0f;
    entity->onCollision = [this, bounce](Entity& self, Entity& other, glm::vec2 pv)
    {
        if (&other == _ball)
        {
            bounce(self, other, pv);
        }
    };
    entity->name = "bottomwall";
    _entities.push_back(std::move(entity));

    /* Ball */
    entity = std::make_unique<Entity>();
    entity->pos = {0.0f, 0.0f};
    entity->size = {0.02f, 0.02f};
    // entity->size = {0.1f, 0.1f};
    entity->flags = Entity::DISPLAY | Entity::PHYSICS;
    entity->color.r = 1.0f;
    entity->color.g = 1.0f;
    entity->color.b = 1.0f;
    entity->v = {0.0f, 0.0f};
    entity->name = "ball";
    _ball = entity.get();
    _entities.push_back(std::move(entity));

    /* Paddles */
    entity = std::make_unique<Entity>();
    entity->pos = {-0.4f, 0.0f};
    entity->size = {0.02f, 0.1f};
    entity->flags = Entity::DISPLAY | Entity::PHYSICS;
    entity->color.r = 1.0f;
    entity->color.g = 0.75f;
    entity->color.b = 0.5f;
    entity->onCollision = [this,bounce](Entity& self, Entity& other, glm::vec2 pv)
    {
        if (&other == _ball)
        {
            bounce(self, other, pv);
        }
        else /* assume wall */
        {
            self.pos += -pv;
        }
    };
    entity->name = "rightpaddle";
    _p1 = entity.get();
    _entities.push_back(std::move(entity));

    entity = std::make_unique<Entity>();
    entity->pos = {0.4f, 0.0f};
    entity->size = {0.02f, 0.1f};
    entity->flags = Entity::DISPLAY | Entity::PHYSICS;
    entity->color.r = 0.5f;
    entity->color.g = 0.75f;
    entity->color.b = 1.0f;
    entity->onCollision = [this,bounce](Entity& self, Entity& other, glm::vec2 pv)
    {
        if (&other == _ball)
        {
            bounce(self, other, pv);
        }
        else /* assume wall */
        {
            self.pos += -pv;
        }
    };
    entity->name = "leftpaddle";
    _p2 = entity.get();
    _entities.push_back(std::move(entity));

    for (auto& e : _entities)
    {
        e->origColor = e->color;
    }

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

/* We start the ball movement after someone hits any key */
void App::onUpdate()
{
    _debugText.clear();

    if (_ball)
    {
        const float movespeed = 0.5;
        if (_keyState.space)
        {
            if (_ball->v.x == 0.0f && _ball->v.y == 0.0f)
            {
                _ball->v = glm::vec2 {(_rng.fnext() * 2.0f) - 1.0f, (_rng.fnext() * 2.0f) - 1.0f};
            }
        }

        if (_keyState.left)
        {
            _ball->pos.x = _ball->pos.y = 0.0f;
            _ball->v.x = _ball->v.y = 0.0f;
        }

        if (glm::length(_ball->v))
        {
            _ball->v = glm::normalize(_ball->v) * movespeed;
        }
    }

    if (_p1)
    {
        if (_keyState.up)
        {
            _p1->v.y = -0.3f;
        }
        else if (_keyState.down)
        {
            _p1->v.y = 0.3f;
        }
        else
        {
            _p1->v.y = 0.0f;
        }
    }

    std::vector<Entity*> entities;
    for (auto& e : _entities)
    {
        if (e->flags & Entity::PHYSICS)
        {
            entities.push_back(e.get());
        }
    }

    for (auto& e : entities)
    {
        e->pos += e->v * dT;
    }

    for (auto& a : entities)
    {
        for (const auto& b : entities)
        {
            if (a != b)
            {
                auto pv = penetrationVector(*a, *b);
                if (pv)
                {
                    if (a->onCollision)
                    {
                        a->onCollision(*a, *b, *pv);
                    }
                }
            }
        }
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

    rcGameScreen.w *= 0.9f;
    rcGameScreen.h *= 0.9f;

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
    auto drawRect = [rcGameScreen, renderer = _renderer](glm::vec2 pos, glm::vec2 size, glm::vec3 col)
    {
        SDL_SetRenderDrawColor(renderer, std::round(col.r * 255.0f), std::round(col.g * 255.0f), std::round(col.b * 255.0f), 0xFF);

        SDL_FRect rc;
        rc.x = (pos.x - (size.x / 2.0f) + 0.5f) * rcGameScreen.w + rcGameScreen.x;
        rc.y = (pos.y - (size.y / 2.0f) + 0.5f) * rcGameScreen.h + rcGameScreen.y;
        rc.w = size.x * rcGameScreen.w;
        rc.h = size.y * rcGameScreen.h;
        SDL_RenderFillRect(renderer, &rc);
    };
    auto drawFrame = [rcGameScreen, renderer = _renderer](glm::vec2 pos, glm::vec2 size, glm::vec3 col)
    {
        SDL_SetRenderDrawColor(renderer, std::round(col.r * 255.0f), std::round(col.g * 255.0f), std::round(col.b * 255.0f), 0xFF);

        SDL_FRect rc;
        rc.x = (pos.x + 0.5f) * rcGameScreen.w + rcGameScreen.x;
        rc.y = (pos.y + 0.5f) * rcGameScreen.h + rcGameScreen.y;
        rc.w = size.x * rcGameScreen.w;
        rc.h = size.y * rcGameScreen.h;
        SDL_RenderRect(renderer, &rc);
    };
    auto drawLine = [rcGameScreen, renderer = _renderer](glm::vec2 a, glm::vec2 b, glm::vec3 col)
    {
        SDL_SetRenderDrawColor(renderer, std::round(col.r * 255.0f), std::round(col.g * 255.0f), std::round(col.b * 255.0f), 0xFF);
        SDL_RenderLine(renderer, (a.x + 0.5f) * rcGameScreen.w + rcGameScreen.x, (a.y + 0.5f) * rcGameScreen.h + rcGameScreen.y,
                       (b.x + 0.5f) * rcGameScreen.w + rcGameScreen.x, (b.y + 0.5f) * rcGameScreen.h + rcGameScreen.y);
    };
    auto drawDigit = [drawRect, renderer = _renderer](char digit, glm::vec2 pos, glm::vec3 col)
    {
        assert(digit >= '0' && digit <= '9');
        const char* charData = FONT_DATA[digit - '0'];
        for (int j = 0; j < 5; j++)
        {
            for (int i = 0; i < 3; i++)
            {
                if (charData[j * 3 + i] != ' ')
                {
                    drawRect(glm::vec2 {i * 0.02f, j * 0.02f} + pos, {0.02f, 0.02f}, col);
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
            drawDigit(digit1 + '0', {scoreLocations[i], -0.48f}, {0.5f, 0.7f, 0.0f});
        }

        auto digit2 = (_scores[i] % 10);
        drawDigit(digit2 + '0', {scoreLocations[i] + 0.07f, -0.48f}, {0.5f, 0.7f, 0.0f});
    }

    /* Entities (they are just rectangles) */
    for (const auto& entity : _entities)
    {
        drawRect(entity->pos, entity->size, entity->color);
    }

    /* Penetration vectors */
    for (const auto& entity : _entities)
    {
        if (entity->pv)
        {
            drawLine(entity->pos, entity->pos + *entity->pv, {1.0f, 0.87f, 0.08f});
        }
    }

    /* Debug text */
    auto debugText = fmt::format("fps={} {}", _fps, _debugText);
    SDL_SetRenderClipRect(_renderer, nullptr);
    SDL_SetRenderDrawColor(_renderer, 255, 255, 64, 255);
    SDL_RenderDebugText(_renderer, 10.0f, 10.0f, debugText.c_str());

    SDL_RenderPresent(_renderer);
}
