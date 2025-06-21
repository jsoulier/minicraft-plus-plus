#pragma once

#include <cstdint>

static constexpr uint64_t Black = 0;
static constexpr uint64_t Red = 700;
static constexpr uint64_t Green = 70;
static constexpr uint64_t Blue = 7;
static constexpr uint64_t Yellow = 770;
static constexpr uint64_t Cyan = 77;
static constexpr uint64_t Magenta = 707;
static constexpr uint64_t White = 777;

uint64_t mppRendererCreateSprite(
    uint64_t color1,
    uint64_t color2,
    uint64_t color3,
    uint64_t color4,
    uint64_t x,
    uint64_t y,
    uint64_t size);

bool mppRendererInit();
void mppRendererQuit();
void mppRendererClear();
void mppRendererPresent();
void mppRendererDraw(uint64_t sprite, float x, float y);