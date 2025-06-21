#pragma once

#include <cstdint>
#include <memory>

enum MppEntityType
{
    MppEntityTypePlayer,
};

class MppEntity
{
public:
    MppEntity();
    virtual void render() = 0;
    virtual void update(uint64_t dt) = 0;
    virtual MppEntityType getType() const = 0;

protected:
    float x;
    float y;
};

std::shared_ptr<MppEntity> mppEntityCreate(MppEntityType type, void* args = nullptr);