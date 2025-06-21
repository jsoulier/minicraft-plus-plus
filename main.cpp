#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "renderer.hpp"

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
    SDL_SetAppMetadata("Minicraft Plus Plus", nullptr, nullptr);
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);

    if (!mppRendererInit())
    {
        SDL_Log("Failed to initialize renderer");
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    mppRendererQuit();
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    uint64_t sprite1 = mppRendererCreateSprite(Red, Green, Blue, Magenta, 0, 0, 16);
    uint64_t sprite2 = mppRendererCreateSprite(Red, Green, Blue, Magenta, 16, 0, 16);

    mppRendererClear();
    mppRendererDraw(sprite1, 32, 32);
    mppRendererDraw(sprite2, 64, 32);
    mppRendererPresent();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        return SDL_APP_FAILURE;

    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
    case SDL_EVENT_MOUSE_WHEEL:
    case SDL_EVENT_MOUSE_MOTION:
        break;
    }

    return SDL_APP_CONTINUE;
}