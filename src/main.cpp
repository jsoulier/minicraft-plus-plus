#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <render/render.hpp>
#include <util.hpp>

int main(int argc, char** argv)
{
    SDL_SetAppMetadata("Minicraft Plus Plus", nullptr, nullptr);
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        MPP_LOG_RELEASE("Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    if (!mppRenderInit())
    {
        MPP_LOG_RELEASE("Failed to initialize render: %s", SDL_GetError());
        return 1;
    }

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
            case SDL_EVENT_MOUSE_WHEEL:
            case SDL_EVENT_MOUSE_MOTION:
                break;
            }
        }

        if (!running)
        {
            break;
        }

        mppRenderSubmit();
    }

    mppRenderQuit();
    SDL_Quit();

    return 0;
}