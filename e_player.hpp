#pragma once

#include "e_mob.hpp"

class MppEntityPlayer : public MppEntityMob
{
public:
    void move(int& dx, int& dy) const override;
    int getType() const override;
};