#include <SDL3/SDL.h>

#include <memory>

#include "e_mob.hpp"
#include "e_player.hpp"
#include "entity.hpp"

MppEntity::MppEntity()
    : x{0.0f}
    , y{0.0f} {}

std::shared_ptr<MppEntity> mppEntityCreate(MppEntityType type, void* args)
{
    std::shared_ptr<MppEntity> entity;

    switch (type)
    {
    case MppEntityTypePlayer:
        entity = std::make_shared<MppEntityPlayer>();
        break;
    }

    if (!entity)
    {
        SDL_Log("Failed to create entity: %d", type);
        return nullptr;
    }

    return entity;
}