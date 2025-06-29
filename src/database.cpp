#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <sqlite3.h>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "database.hpp"

static constexpr uint32_t GetVersion(uint32_t major, uint32_t minor, uint32_t patch)
{
    return major << 24 | minor << 16 | patch << 0;
}

static constexpr uint32_t MajorVersion = 0;
static constexpr uint32_t MinorVersion = 0;
static constexpr uint32_t PatchVersion = 0;
static constexpr uint32_t BuildVersion = GetVersion(MajorVersion, MinorVersion, PatchVersion);
static constexpr std::string_view SaveFileExtension = ".sqlite3";
static constexpr int MetadataId = 0;

static bool hasConnection;
static sqlite3* connection;
static sqlite3_stmt* updateTicks;
static sqlite3_stmt* selectTicks;
static sqlite3_stmt* insertEntity;
static sqlite3_stmt* updateEntity;
static sqlite3_stmt* removeEntity;
static sqlite3_stmt* updateTile;
static sqlite3_stmt* selectEntities;
static sqlite3_stmt* selectTiles;
static uint32_t savedTicks;

static bool CreateTables()
{
    const char* createTablesSQL =
        "CREATE TABLE IF NOT EXISTS metadata ("
        "    id INTEGER PRIMARY KEY,"
        "    ticks INTEGER NOT NULL"
        ");"
        ""
        "CREATE TABLE IF NOT EXISTS entities ("
        "    version INTEGER NOT NULL,"
        "    type INTEGER NOT NULL,"
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    x FLOAT NOT NULL,"
        "    y INTEGER NOT NULL,"
        "    z FLOAT NOT NULL,"
        "    data BLOB"
        ");"
        ""
        "CREATE TABLE IF NOT EXISTS tiles ("
        "    type INTEGER NOT NULL,"
        "    x INTEGER NOT NULL,"
        "    y INTEGER NOT NULL,"
        "    z INTEGER NOT NULL,"
        "    health INTEGER NOT NULL,"
        "    ticks INTEGER NOT NULL,"
        "    PRIMARY KEY (x, y, z)"
        ");";

    if (sqlite3_exec(connection, createTablesSQL, 0, nullptr, nullptr) != SQLITE_OK)
    {
        SDL_Log("Failed to execute SQL: %s", sqlite3_errmsg(connection));
        return false;
    }

    return true;
}

static bool CreateStatements()
{
    const char* insertEntitySQL =
        "";

    return true;
}

bool DatabaseInit(const std::string_view& name)
{
    std::string path = std::format("{}.{}", name, SaveFileExtension);

    if (sqlite3_open(path.data(), &connection) != SQLITE_OK)
    {
        SDL_Log("Failed to open database: %s, %s", path.data(), sqlite3_errmsg(connection));
        return false;
    }

    if (!CreateTables())
    {
        SDL_Log("Failed to create tables");
        return false;
    }

    if (!CreateStatements())
    {
        SDL_Log("Failed to create statements");
        return false;
    }

    hasConnection = true;

    if (sqlite3_exec(connection, "BEGIN;", 0, 0, 0) != SQLITE_OK)
    {
        SDL_Log("Failed to start transaction: %s", sqlite3_errmsg(connection));
    }

    return true;
}

void DatabaseQuit()
{
    if (!hasConnection)
    {
        return;
    }

    if (sqlite3_exec(connection, "COMMIT;", 0, 0, 0) != SQLITE_OK)
    {
        SDL_Log("Failed to end transaction: %s", sqlite3_errmsg(connection));
    }

    sqlite3_finalize(updateTicks);
    sqlite3_finalize(selectTicks);
    sqlite3_finalize(insertEntity);
    sqlite3_finalize(updateEntity);
    sqlite3_finalize(removeEntity);
    sqlite3_finalize(updateTile);
    sqlite3_finalize(selectEntities);
    sqlite3_finalize(selectTiles);

    updateTicks = nullptr;
    selectTicks = nullptr;
    insertEntity = nullptr;
    updateEntity = nullptr;
    removeEntity = nullptr;
    updateTile = nullptr;
    selectEntities = nullptr;
    selectTiles = nullptr;

    sqlite3_close(connection);

    connection = nullptr;
    hasConnection = false;
}

void DatabaseSubmit(uint32_t ticks)
{
    if (!hasConnection)
    {
        return;
    }

    if (sqlite3_exec(connection, "COMMIT;", 0, 0, 0) != SQLITE_OK)
    {
        SDL_Log("Failed to end transaction: %s", sqlite3_errmsg(connection));
    }

    if (sqlite3_exec(connection, "BEGIN;", 0, 0, 0) != SQLITE_OK)
    {
        SDL_Log("Failed to start transaction: %s", sqlite3_errmsg(connection));
    }
}

uint32_t DatabaseGetTicks()
{
    return savedTicks;
}

void DatabaseAdd(std::shared_ptr<Entity>& entity)
{
    if (!hasConnection)
    {
        return;
    }
}

void DatabaseAdd(const Tile& tile, int x, int y, int z)
{
    if (!hasConnection)
    {
        return;
    }
}

void DatabaseRemove(std::shared_ptr<Entity>& entity)
{
    if (!hasConnection)
    {
        return;
    }
}

void DatabaseGet(const std::function<void(std::shared_ptr<Entity>& entity)>& callback)
{
    if (!hasConnection)
    {
        return;
    }
}

void DatabaseGet(const std::function<void(const Tile& tile, int x, int y, int z)>& callback)
{
    if (!hasConnection)
    {
        return;
    }
}

bool DatabaseVisit(void* data, uint32_t size, int major, int minor, int patch)
{
    if (!hasConnection)
    {
        return;
    }

    return true;
}