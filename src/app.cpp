#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <string_view>
#include "database.hpp"
#include "renderer.hpp"

static constexpr std::string_view SaveFileName = "minicraft++";

 /* TODO: log to a file */
static constexpr std::string_view LogFile = "minicraft++.log";

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

    if (!DatabaseInit(SaveFileName))
    {
        SDL_Log("Failed to initialize database: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    DatabaseQuit();
    RendererQuit();
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    static float i = 0.0f;
    i += 0.002f;

    RendererMove({}, i);
    RendererDraw(RendererModelGrass, {}, -i);
    RendererDraw(RendererModelGrass, {100.0f, 0.0f, 0.0f}, -i);
    RendererDraw(RendererModelGrass, {-100.0f, 0.0f, 0.0f}, -i);
    RendererDraw(RendererModelGrass, {0.0f, 0.0f, 100.0f}, -i);
    RendererDraw(RendererModelGrass, {0.0f, 0.0f, -100.0f}, -i);
    RendererDraw(RendererModelGrass, {-100.0f, 0.0f, -100.0f}, -i);
    RendererDraw(RendererModelGrass, {-100.0f, 0.0f, 100.0f}, -i);
    RendererDraw(RendererModelGrass, {100.0f, 0.0f, -100.0f}, -i);
    RendererDraw(RendererModelGrass, {100.0f, 0.0f, 100.0f}, -i);
    RendererDraw("Testing\nTesting", {100.0f, 100.0f}, {1.0f, 0.0f, 1.0f, 1.0f}, 16);
    RendererDraw("Testing\nTesting v2", {100.0f, 600.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, 16);
    RendererDraw("Testing\nTesting", {800.0f, 600.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, 16);
    RendererDraw("Testing\nTesting v2", {800.0f, 100.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, 16);
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