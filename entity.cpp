#include <SDL3/SDL.h>

#include <memory>

#include "e_mob.hpp"
#include "e_player.hpp"
#include "entity.hpp"

MppEntity::MppEntity()
    : id{-1}
    , x{0.0f}
    , y{0.0f} {}

int64_t MppEntity::getId() const
{
    return id;
}

float MppEntity::getX() const
{
    return x;
}

float MppEntity::getY() const
{
    return y;
}

void MppEntity::setId(int64_t id)
{
    this->id = id;
}

void MppEntity::setX(float x)
{
    this->x = x;
}

void MppEntity::setY(float y)
{
    this->y = y;
}

std::shared_ptr<MppEntity> mppEntityCreate(int type, void* args)
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