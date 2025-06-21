#pragma once

#include <memory>

class MppEntity;

static constexpr int MppLevelWidth = 256;
static constexpr int MppLevelDepth = 1;

bool mppLevelInit(bool hasDatabase);
void mppLevelQuit();
void mppLevelInsert(std::shared_ptr<MppEntity>& entity);
void mppLevelRender();
void mppLevelUpdate(uint64_t dt, uint64_t time);
void mppLevelCommit();