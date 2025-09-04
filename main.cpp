#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "app.hpp"
#include <fmt/format.h>

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    App* app = new App;
    auto ret = app->onInit(argc, argv);
    if (ret != SDL_APP_CONTINUE)
    {
        delete app;
    }
    else
    {
        *appstate = app;
    }
    return ret;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    App* app = reinterpret_cast<App*>(appstate);
    return app->onEvent(event);
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    App* app = reinterpret_cast<App*>(appstate);
    return app->onIterate();
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    App* app = reinterpret_cast<App*>(appstate);
    if (app)
    {
        app->onQuit(result);
    }
    delete app;
}
