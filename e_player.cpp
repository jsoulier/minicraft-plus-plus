#include <SDL3/SDL.h>

#include "e_player.hpp"

void MppEntityPlayer::move(int& dx, int& dy) const
{
    const bool* keys = SDL_GetKeyboardState(nullptr);
    if (keys[SDL_SCANCODE_W])
    {
        dy--;
    }
    if (keys[SDL_SCANCODE_S])
    {
        dy++;
    }
    if (keys[SDL_SCANCODE_A])
    {
        dx--;
    }
    if (keys[SDL_SCANCODE_D])
    {
        dx++;
    }
}

int MppEntityPlayer::getType() const
{
    return MppEntityTypePlayer;
}