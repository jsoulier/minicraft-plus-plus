#pragma once

#include <memory>
#include <functional>

class MppEntity;

bool mppDatabaseInit();
void mppDatabaseQuit();
void mppDatabaseCommit();
void mppDatabaseInsert(std::shared_ptr<MppEntity>& entity, int level);
void mppDatabaseSelect(const std::function<void(std::shared_ptr<MppEntity>&, int)>& function);