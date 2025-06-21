#pragma once

#include <cstdint>

#include "entity.hpp"

class MppEntityMob : public MppEntity
{
public:
    void render() override;
    void update(uint64_t dt, uint64_t time) override;

protected:
    virtual void move(int& dx, int& dy) const = 0;
    virtual float getSpeed() const { return 1.0f; }
};