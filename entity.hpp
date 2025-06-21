#pragma once

#include <cstdint>
#include <memory>

static constexpr int MppEntityTypePlayer = 0;

class MppEntity
{
public:
    MppEntity();

    virtual void render() = 0;
    virtual void update(uint64_t dt, uint64_t time) = 0;

    virtual void serialize() {}
    virtual int getType() const = 0;

    int64_t getId() const;
    float getX() const;
    float getY() const;
    int getLevel() const;

    void setId(int64_t id);
    void setX(float x);
    void setY(float y);
    void setLevel(int level);

protected:
    int64_t id;
    float x;
    float y;
    int level;
};

std::shared_ptr<MppEntity> mppEntityCreate(int type, void* args = nullptr);