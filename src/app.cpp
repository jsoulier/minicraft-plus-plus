#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "renderer.hpp"

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
#ifndef NDEBUG
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
#endif

    if (!RendererInit())
    {
        SDL_Log("Failed to initialize renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    RendererQuit();
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    static float i = 0.0f;
    i += 0.01f;

    RendererMove({}, i);
    RendererDraw(RendererModelGrass, {}, -i);
    RendererDraw("Hello World!", {100.0f, 100.0f}, {1.0f, 0.0f, 1.0f, 1.0f}, 16);
    RendererSubmit();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}