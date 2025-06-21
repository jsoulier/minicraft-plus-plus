#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "database.hpp"
#include "renderer.hpp"
#include "entity.hpp"

static std::shared_ptr<MppEntity> player;

static uint64_t t1;
static uint64_t t2;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
    SDL_SetAppMetadata("Minicraft Plus Plus", nullptr, nullptr);
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);

    if (!mppRendererInit())
    {
        SDL_Log("Failed to initialize renderer");
        return SDL_APP_FAILURE;
    }

    bool hasDatabase = mppDatabaseInit();

    t2 = SDL_GetTicks();
    t1 = t2;

    if (hasDatabase)
    {
        mppDatabaseSelect([&](std::shared_ptr<MppEntity>& entity, int level)
        {
            player = entity;
        });
    }

    if (!player)
    {
        player = mppEntityCreate(MppEntityTypePlayer);
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    mppDatabaseQuit();
    mppRendererQuit();
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    t2 = SDL_GetTicks();
    uint64_t dt = t2 - t1;
    t1 = t2;

    player->update(dt);

    mppDatabaseInsert(player, 0);

    uint64_t sprite1 = mppRendererCreateSprite(Red, Green, Blue, Magenta, 0, 0, 16);
    uint64_t sprite2 = mppRendererCreateSprite(Red, Green, Blue, Magenta, 16, 0, 16);

    mppRendererClear();
    mppRendererDraw(sprite1, 32, 32);
    mppRendererDraw(sprite2, 64, 32);
    mppRendererDraw("testing", 128, 128, Red, 12);
    mppRendererDraw("testing", 64, 128, Green, 12);
    mppRendererDraw("testing", 192, 128, Blue, 12);
    mppRendererDraw("testing", 128, 64, Red, 6);
    mppRendererDraw("testing", 64, 64, Green, 6);
    mppRendererDraw("testing", 192, 64, Blue, 6);
    mppRendererDraw("testing", 128, 32, Red, 6);
    mppRendererDraw("testing", 64, 32, Green, 6);
    mppRendererDraw("testing", 192, 32, Blue, 6);

    player->render();

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