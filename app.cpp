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

    _startSound.load("start.ogg");
    _bounceSound.load("bounce.ogg");
    _loseSound.load("lose.ogg");

    SDL_AudioSpec inAudioSpec;
    inAudioSpec.format = SDL_AUDIO_S16LE;
    inAudioSpec.channels = _startSound.channels();
    inAudioSpec.freq = _startSound.sampleRate();

    _audioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &inAudioSpec, nullptr, nullptr);
    if (!_audioStream)
    {
        fmt::println(stderr, "Failed to create audio stream");
        abort();
    }
    if (!SDL_ResumeAudioStreamDevice(_audioStream))
    {
        fmt::println(stderr, "Failed to resume audio stream playback");
        abort();
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
    entity->size = {0.1f, GAME_HEIGHT + 0.2f};
    entity->pos = {(-GAME_WIDTH / 2.0f) - (entity->size.x / 2.0f), 0.0f};
    entity->flags = Entity::PHYSICS;
    entity->color.r = 1.0f;
    entity->color.g = 0.5f;
    entity->color.b = 1.0f;
    entity->onCollision = [this, bounce](Entity& self, Entity& other, glm::vec2 pv)
    {
        if (&other == _ball)
        {
            _scores[1]++;
            reset();
            playSound(_loseSound);
        }
    };
    entity->name = "leftwall";
    _entities.push_back(std::move(entity));

    /* Right wall */
    entity = std::make_unique<Entity>();
    entity->size = {0.1f, GAME_HEIGHT + 0.2f};
    entity->pos = {(GAME_WIDTH / 2.0f) + (entity->size.x / 2.0f), 0.0f};
    entity->flags = Entity::PHYSICS;
    entity->color.r = 1.0f;
    entity->color.g = 0.5f;
    entity->color.b = 1.0f;
    entity->onCollision = [this, bounce](Entity& self, Entity& other, glm::vec2 pv)
    {
        if (&other == _ball)
        {
            _scores[0]++;
            reset();
            playSound(_loseSound);
        }
    };
    entity->name = "rightwall";
    _entities.push_back(std::move(entity));

    /* Top wall */
    entity = std::make_unique<Entity>();
    entity->size = {GAME_WIDTH, 0.1f};
    entity->pos = {0, -0.5f - (entity->size.y / 2.0f)};
    entity->flags = Entity::PHYSICS;
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
    entity->size = {GAME_WIDTH, 0.1f};
    entity->pos = {0.0f, 0.5f + (entity->size.y / 2.0f)};
    entity->flags = Entity::PHYSICS;
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
    entity->size = {0.05f, 0.05f};
    entity->flags = Entity::DISPLAY | Entity::PHYSICS;
    entity->color.r = 1.0f;
    entity->color.g = 1.0f;
    entity->color.b = 1.0f;
    entity->v = {0.0f, 0.0f};
    entity->onUpdate = [this](Entity& ball, const Keystate& keyState)
    {
        if (keyState.space)
        {
            if (ball.v.x == 0.0f && ball.v.y == 0.0f)
            {
                do
                {
                    ball.v = glm::vec2 {(_rng.fnext() * 2.0f) - 1.0f, (_rng.fnext() * 2.0f) - 1.0f};
                } while (ball.v.x < 0.01f);
                playSound(_startSound);
            }
        }

        if (glm::length(ball.v))
        {
            ball.v = glm::normalize(ball.v) * BALL_SPEED;
        }
    };
    entity->name = "ball";
    _ball = entity.get();
    _entities.push_back(std::move(entity));

    /* Paddles */
    entity = std::make_unique<Entity>();
    entity->pos = {-(GAME_WIDTH / 2.0f) + 0.1f, 0.0f};
    entity->size = {_ball->size.x, 0.2f};
    entity->flags = Entity::DISPLAY | Entity::PHYSICS;
    entity->color.r = 1.0f;
    entity->color.g = 0.75f;
    entity->color.b = 0.5f;
    entity->onCollision = [this, bounce](Entity& self, Entity& other, glm::vec2 pv)
    {
        if (&other == _ball)
        {
            bounce(self, other, pv);
            playSound(_bounceSound);
        }
        else /* assume wall */
        {
            self.pos += -pv;
        }
    };
    entity->onUpdate = [](Entity& p1, const Keystate& keyState)
    {
        if (keyState.up)
        {
            p1.v.y = -PADDLE_SPEED;
        }
        else if (keyState.down)
        {
            p1.v.y = PADDLE_SPEED;
        }
        else
        {
            p1.v.y = 0.0f;
        }
    };
    entity->name = "rightpaddle";
    _p1 = entity.get();
    _entities.push_back(std::move(entity));

    entity = std::make_unique<Entity>();
    entity->pos = {(GAME_WIDTH / 2.0f) - 0.1f, 0.0f};
    entity->size = {_ball->size.x, 0.2f};
    entity->flags = Entity::DISPLAY | Entity::PHYSICS;
    entity->color.r = 1.0f;
    entity->color.g = 0.5f;
    entity->color.b = 1.0f;
    entity->onCollision = [this, bounce](Entity& self, Entity& other, glm::vec2 pv)
    {
        if (&other == _ball)
        {
            bounce(self, other, pv);
            playSound(_bounceSound);
        }
        else /* assume wall */
        {
            self.pos += -pv;
        }
    };
    entity->onUpdate = [this](Entity& self, const Keystate& keyState)
    {
        if (_ball->v.x > 0.0f && _ball->pos.y < self.pos.y)
        {
            self.v.y = -PADDLE_SPEED;
        }
        else if (_ball->v.x > 0.0f && _ball->pos.y > self.pos.y)
        {
            self.v.y = PADDLE_SPEED;
        }
        else if (_ball->v.x < 0.0f && _ball->pos.y < self.pos.y)
        {
            self.v.y = PADDLE_SPEED;
        }
        else if (_ball->v.x < 0.0f && _ball->pos.y > self.pos.y)
        {
            self.v.y = -PADDLE_SPEED;
        }
        else
        {
            self.v.y = 0.0f;
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
    SDL_DestroyAudioStream(_audioStream);
}

/* We start the ball movement after someone hits any key */
void App::onUpdate()
{
    _debugText.clear();

    std::vector<Entity*> entities;
    for (auto& e : _entities)
    {
        if (e->flags & Entity::PHYSICS)
        {
            entities.push_back(e.get());
        }
        if (e->onUpdate)
        {
            e->onUpdate(*e, _keyState);
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

static SDL_FRect getScreenSize(SDL_Renderer* renderer)
{
    int w, h;
    SDL_GetRenderOutputSize(renderer, &w, &h);
    return {0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)};
}

struct Transformation
{
    glm::vec2 scale;
    glm::vec2 translation;
};

float transform(float scale, float translation, float x)
{
    return (x * scale) + translation;
}

glm::vec2 transform(const Transformation& t, glm::vec2 p)
{
    return {transform(t.scale.x, t.translation.x, p.x), transform(t.scale.y, t.translation.y, p.y)};
}

SDL_FRect transform(const Transformation& t, SDL_FRect rc)
{
    rc.x = transform(t.scale.x, t.translation.x, rc.x);
    rc.y = transform(t.scale.y, t.translation.y, rc.y);
    rc.w = rc.w * t.scale.x;
    rc.h = rc.h * t.scale.y;
    return rc;
}

void App::onRender()
{
    auto screen = getScreenSize(_renderer);

    /* Clear screen */
    SDL_SetRenderDrawColor(_renderer, std::round(COLOR_BACKGROUND.r * 255), std::round(COLOR_BACKGROUND.g * 255), std::round(COLOR_BACKGROUND.b * 255), 0xFF);
    SDL_RenderClear(_renderer);

    /* Determine gameScreen geometry */
    SDL_FRect gameScreen;
    if (screen.w / screen.h >= (GAME_WIDTH / GAME_HEIGHT))
    {
        gameScreen.h = screen.h * GAME_SCALE;
        gameScreen.w = gameScreen.h * GAME_WIDTH;
    }
    else
    {
        gameScreen.w = screen.w * GAME_SCALE;
        gameScreen.h = gameScreen.w / GAME_WIDTH;
    }
    gameScreen.x = (screen.w - gameScreen.w) / 2.0f;
    gameScreen.y = (screen.h - gameScreen.h) / 2.0f;

    SDL_SetRenderDrawColor(_renderer, std::round(COLOR_GAMESCREEN.r), std::round(COLOR_GAMESCREEN.g), std::round(COLOR_GAMESCREEN.b), 0xFF);
    SDL_RenderFillRect(_renderer, &gameScreen);

    SDL_Rect clipRect;
    clipRect.x = std::round(gameScreen.x);
    clipRect.y = std::round(gameScreen.y);
    clipRect.w = std::round(gameScreen.w);
    clipRect.h = std::round(gameScreen.h);
    SDL_SetRenderClipRect(_renderer, &clipRect);

    Transformation screenT;
    screenT.scale.x = screen.w / GAME_WIDTH; /* width = 1.77 */
    screenT.scale.y = screen.h;              /* height = 1.0 */
    screenT.translation.x = screen.w / 2.0f; /* origin at middle of screen */
    screenT.translation.y = screen.h / 2.0f; /* origin at middle of screen */

    Transformation gameT;
    gameT.scale.x = gameScreen.w / screen.w;
    gameT.scale.y = gameScreen.h / screen.h;
    gameT.translation.x = gameScreen.x;
    gameT.translation.y = gameScreen.y;

    auto drawRect = [this, screenT, gameT](glm::vec2 p, glm::vec2 s, glm::vec3 c)
    {
        SDL_FRect rc;
        rc.x = p.x;
        rc.y = p.y;
        rc.w = s.x;
        rc.h = s.y;
        rc = transform(gameT, transform(screenT, rc));

        SDL_SetRenderDrawColor(_renderer, std::round(c.r * 255), std::round(c.g * 255), std::round(c.b * 255), 0xFF);
        SDL_RenderFillRect(_renderer, &rc);
    };
    auto drawFrame = [this, screenT, gameT](glm::vec2 p, glm::vec2 s, glm::vec3 c)
    {
        SDL_FRect rc;
        rc.x = p.x;
        rc.y = p.y;
        rc.w = s.x;
        rc.h = s.y;
        rc = transform(gameT, transform(screenT, rc));

        SDL_SetRenderDrawColor(_renderer, std::round(c.r * 255), std::round(c.g * 255), std::round(c.b * 255), 0xFF);
        SDL_RenderRect(_renderer, &rc);
    };
    auto drawLine = [this, screenT, gameT](glm::vec2 a, glm::vec2 b, glm::vec3 col)
    {
        a = transform(gameT, transform(screenT, a));
        b = transform(gameT, transform(screenT, b));

        SDL_SetRenderDrawColor(_renderer, std::round(col.r * 255.0f), std::round(col.g * 255.0f), std::round(col.b * 255.0f), 0xFF);
        SDL_RenderLine(_renderer, a.x, a.y, b.x, b.y);
    };
    auto drawDigit = [this, drawRect](char digit, glm::vec2 pos, glm::vec3 col)
    {
        assert(digit >= '0' && digit <= '9');
        const char* charData = FONT_DATA[digit - '0'];
        for (int j = 0; j < 5; j++)
        {
            for (int i = 0; i < 3; i++)
            {
                if (charData[j * 3 + i] != ' ')
                {
                    drawRect(glm::vec2 {i * SCORE_SIZE, j * SCORE_SIZE} + pos, {SCORE_SIZE, SCORE_SIZE}, col);
                }
            }
        }
    };

    /* Score */
    static const float scoreLocations[] = {-(GAME_WIDTH / 2.0f) + (SCORE_SIZE * 4.0f), (GAME_WIDTH / 2.0f) - (SCORE_SIZE * 8.0f)};
    for (int i = 0; i < 2; i++)
    {
        auto digit1 = (_scores[i] / 10) % 10;
        if (digit1)
        {
            drawDigit(digit1 + '0', {scoreLocations[i], -0.48f}, COLOR_SCORE);
        }

        auto digit2 = (_scores[i] % 10);
        drawDigit(digit2 + '0', {scoreLocations[i] + (SCORE_SIZE * 4.0f), -0.48f}, COLOR_SCORE);
    }

    /* Entities (they are just rectangles) */
    for (const auto& entity : _entities)
    {
        if (entity->flags & Entity::DISPLAY)
        {
            drawRect({entity->pos.x - (entity->size.x / 2.0f), entity->pos.y - (entity->size.y / 2.0f)}, entity->size, entity->color);
        }
    }

    /* Debug text */
    auto debugText = fmt::format("fps={} {}", _fps, _debugText);
    SDL_SetRenderClipRect(_renderer, nullptr);
    SDL_SetRenderDrawColor(_renderer, std::round(COLOR_DEBUGTEXT.r * 255), std::round(COLOR_DEBUGTEXT.g * 255), std::round(COLOR_DEBUGTEXT.b * 255), 0xFF);
    SDL_RenderDebugText(_renderer, 10.0f, 10.0f, debugText.c_str());

    /* Show into screen */
    SDL_RenderPresent(_renderer);
}

void App::playSound(const Sfx& sound)
{
    SDL_PutAudioStreamData(_audioStream, sound.samples(), sound.size());
}

