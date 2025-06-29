#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
#include <utility>

class Entity;
class Tile;

bool DatabaseInit(const std::string_view& name);
void DatabaseQuit();
void DatabaseSubmit(uint32_t ticks);
uint32_t DatabaseGetTicks();
void DatabaseAdd(std::shared_ptr<Entity>& entity);
void DatabaseAdd(const Tile& tile, int x, int y, int z);
void DatabaseRemove(std::shared_ptr<Entity>& entity);
void DatabaseGet(const std::function<void(std::shared_ptr<Entity>& entity)>& callback);
void DatabaseGet(const std::function<void(const Tile& tile, int x, int y, int z)>& callback);
bool DatabaseVisit(void* data, uint32_t size, int major, int minor, int patch);

template<typename T, typename... Args>
void DatabaseVisit(T& data, int major, int minor, int patch, Args&&... args)
{
    if (!DatabaseVisit(std::addressof(data), sizeof(T), major, minor, patch))
    {
        data = T{std::forward<Args>(args)...};
    }
}

template<typename T>
void DatabaseVisit(T& data)
{
    DatabaseVisit(data, 0, 0, 0);
}