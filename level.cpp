#include <SDL3/SDL.h>

#include <cassert>
#include <memory>
#include <vector>

#include "database.hpp"
#include "entity.hpp"
#include "level.hpp"

struct Level
{
    std::vector<std::shared_ptr<MppEntity>> entities;
};

static Level levels[MppLevelDepth];
static int currentLevel = -1;

bool mppLevelInit(bool hasDatabase)
{
    if (hasDatabase)
    {
        mppDatabaseSelect(mppLevelInsert);

        if (currentLevel == -1)
        {
            SDL_Log("Failed to load level: %d", currentLevel);
        }
    }

    if (currentLevel == -1)
    {
        /* TODO: noise */

        currentLevel = 0;

        levels[currentLevel].entities.push_back(mppEntityCreate(MppEntityTypePlayer));
    }

    return true;
}

void mppLevelQuit()
{
    for (int i = 0; i < MppLevelDepth; i++)
    {
        levels[i].entities.clear();
    }
}

void mppLevelInsert(std::shared_ptr<MppEntity>& entity)
{
    int level = entity->getLevel();
    if (level == -1)
    {
        level = currentLevel;
    }

    if (entity->getType() == MppEntityTypePlayer)
    {
        currentLevel = level;
    }

    assert(currentLevel != -1);

    levels[currentLevel].entities.push_back(entity);
}

void mppLevelRender()
{
    assert(currentLevel != -1);

    Level& level = levels[currentLevel];

    for (std::shared_ptr<MppEntity>& entity : level.entities)
    {
        entity->render();
    }
}

void mppLevelUpdate(uint64_t dt, uint64_t time)
{
    assert(currentLevel != -1);

    Level& level = levels[currentLevel];

    for (std::shared_ptr<MppEntity>& entity : level.entities)
    {
        entity->update(dt, time);
    }
}

void mppLevelCommit()
{
    assert(currentLevel != -1);

    Level& level = levels[currentLevel];

    for (std::shared_ptr<MppEntity>& entity : level.entities)
    {
        mppDatabaseInsert(entity);
    }
}