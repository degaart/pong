#include "app.hpp"

#include <algorithm>
#include <assert.h>
#include <fmt/format.h>

App::App()
    : _window(nullptr), _renderer(nullptr), _prevTime(0.0), _lag(0.0),
      _theta(0.0f), _fpsTimer(0.0), _frames(0), _fps(0)
{
    memset(&_keyState, 0, sizeof(_keyState));
}

SDL_AppResult App::onInit(int argc, char** argv)
{
    auto ret = SDL_CreateWindowAndRenderer(
        "Rasterization", SCREEN_WIDTH, SCREEN_HEIGHT, 0, &_window, &_renderer);
    if (!ret)
    {
        return SDL_APP_FAILURE;
    }

    Rng rng(SDL_GetTicks());

    static constexpr auto POLYPRIME_WIDTH = 256.0f;
    static constexpr auto POLYPRIME_HEIGHT = 128.0f;

    Polygon2D polyPrime;
    polyPrime.visible = true;
    polyPrime.pos.x = (SCREEN_WIDTH - POLYPRIME_WIDTH) / 2.0f;
    polyPrime.pos.y = (SCREEN_HEIGHT - POLYPRIME_HEIGHT) / 2.0f;
    polyPrime.velocity.x = polyPrime.velocity.y = 0.0f;
    polyPrime.color.r = 237.0f / 255.0f;
    polyPrime.color.g = 157.0f / 255.0f;
    polyPrime.color.b = 100.0f / 255.0f;
    polyPrime.vertices.emplace_back(0.0f, -POLYPRIME_HEIGHT / 2.0f);
    polyPrime.vertices.emplace_back(POLYPRIME_WIDTH / 2.0f,
                                    -POLYPRIME_HEIGHT / 2.0f);
    polyPrime.vertices.emplace_back(-POLYPRIME_WIDTH / 2.0f,
                                    POLYPRIME_HEIGHT / 2.0f);

    _polygons.push_back(polyPrime);

    for (int i = 0; i < 10; i++)
    {
        auto poly = createPolygon(rng, 3 + (rng.next() % 5),
                                  (rng.fnext() * 128.0f) - 64.0f);
        poly.visible = true;
        poly.pos.x = SCREEN_WIDTH * rng.fnext();
        poly.pos.y = SCREEN_HEIGHT * rng.fnext();
        poly.velocity.x = poly.velocity.y = 0.0f;
        poly.color.r = rng.fnext();
        poly.color.g = rng.fnext();
        poly.color.b = rng.fnext();
        _polygons.push_back(poly);
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
    if (endTime - beginTime < 1.0/FPS)
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

void App::drawPolygon(const Polygon2D& poly)
{
    SDL_SetRenderDrawColorFloat(_renderer, poly.color.r, poly.color.g,
                                poly.color.b, 1.0f);

    for (int i = 0; i < poly.vertices.size() - 1; i++)
    {
        SDL_RenderLine(_renderer, poly.vertices[i].x + poly.pos.x,
                       poly.vertices[i].y + poly.pos.y,
                       poly.vertices[i + 1].x + poly.pos.x,
                       poly.vertices[i + 1].y + poly.pos.y);
    }

    SDL_RenderLine(_renderer, poly.vertices[0].x + poly.pos.x,
                   poly.vertices[0].y + poly.pos.y,
                   poly.vertices[poly.vertices.size() - 1].x + poly.pos.x,
                   poly.vertices[poly.vertices.size() - 1].y + poly.pos.y);
}

Polygon2D App::createPolygon(Rng& rng, int vertCount, float size)
{
    assert(vertCount >= 3);

    std::vector<float> angles(vertCount);
    for (int i = 0; i < vertCount; i++)
    {
        angles[i] = rng.fnext() * 2.0f * M_PI;
    }

    std::sort(angles.begin(), angles.end());

    Polygon2D result;
    result.vertices.reserve(vertCount);
    for (float a : angles)
    {
        float r = 0.5f + 0.5f * rng.fnext(); /* radius [0.5f,1.0f) */
        float x = r * std::cos(a);
        float y = r * std::sin(a);
        result.vertices.emplace_back(x * size, y * size);
    }

    return result;
}

Polygon2D App::rotatePolygon(Polygon2D poly, float theta)
{
    for (auto& vert : poly.vertices)
    {
        vert.x = vert.x * std::cos(theta) - vert.y * std::sin(theta);
        vert.y = vert.x * std::sin(theta) + vert.y * std::cos(theta);
    }
    return poly;
}

void App::onUpdate()
{
    auto velocity = std::min(640.0f / 5.0f, 480.0f / 5.0f);
    if (_keyState.up)
    {
        _polygons[0].pos.y -= velocity * dT;
    }
    else if (_keyState.down)
    {
        _polygons[0].pos.y += velocity * dT;
    }

    if (_keyState.left)
    {
        _polygons[0].pos.x -= velocity * dT;
    }
    else if (_keyState.right)
    {
        _polygons[0].pos.x += velocity * dT;
    }

    if (_keyState.space)
    {
        _theta += ((2 * M_PI) / 10.0f) * dT;
        if (_theta > 2 * M_PI)
        {
            _theta -= 2 * M_PI;
        }
    }
}

void App::onRender()
{
    SDL_SetRenderDrawColor(_renderer, 0x64, 0x95, 0xED, 0xFF);
    SDL_RenderClear(_renderer);

    SDL_FRect clipRect;
    if (((float)SCREEN_WIDTH / SCREEN_HEIGHT) > 4.0f / 3.0f)
    {
        clipRect.w = (SCREEN_HEIGHT * 4.0f) / 3.0f;
        clipRect.h = SCREEN_HEIGHT;
        clipRect.x = (SCREEN_WIDTH - clipRect.w) / 2.0f;
        clipRect.y = 0.0;
    }
    else
    {
        clipRect.w = SCREEN_WIDTH;
        clipRect.h = (clipRect.w * 3.0f) / 4.0f;
        clipRect.x = 0.0f;
        clipRect.y = (SCREEN_HEIGHT - clipRect.h) / 2.0f;
    }

    SDL_SetRenderDrawColor(_renderer, 255, 255, 255, 255);
    SDL_RenderRect(_renderer, &clipRect);

    SDL_Rect iClipRect;
    iClipRect.w = clipRect.w;
    iClipRect.h = clipRect.h;
    iClipRect.x = clipRect.x;
    iClipRect.y = clipRect.y;
    SDL_SetRenderClipRect(_renderer, &iClipRect);

    for (int i = 0; i < _polygons.size(); i++)
    {
        if (false && i == 0)
        {
            auto rotated = rotatePolygon(_polygons[i], _theta);
            drawPolygon(rotated);
        }
        else
        {
            drawPolygon(_polygons[i]);
        }
    }

    auto debugText =
        fmt::format("fps={} pos={:0.1f},{:0.1f} theta={:0.1f}", _fps,
                    _polygons[0].pos.x, _polygons[0].pos.y, _theta);
    SDL_SetRenderClipRect(_renderer, nullptr);
    SDL_SetRenderDrawColor(_renderer, 255, 255, 64, 255);
    SDL_RenderDebugText(_renderer, 10.0f, 10.0f, debugText.c_str());

    SDL_RenderPresent(_renderer);
}
