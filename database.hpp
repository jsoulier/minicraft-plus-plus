#pragma once

#include <cstdint>
#include <memory>
#include <functional>

class MppEntity;

bool mppDatabaseInit();
void mppDatabaseQuit();
void mppDatabaseCommit();
void mppDatabaseSetTime(int64_t time);
int64_t mppDatabaseGetTime();
void mppDatabaseInsert(std::shared_ptr<MppEntity>& entity);
void mppDatabaseSelect(const std::function<void(std::shared_ptr<MppEntity>&)>& function);