#pragma once

#include <cstdint>

static constexpr uint64_t Black = 0;
static constexpr uint64_t Red = 500;
static constexpr uint64_t Green = 50;
static constexpr uint64_t Blue = 5;
static constexpr uint64_t Yellow = 550;
static constexpr uint64_t Cyan = 55;
static constexpr uint64_t Magenta = 505;
static constexpr uint64_t White = 555;

/* TODO: make constexpr */
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
void mppRendererDraw(const char* text, float x, float y, int color, int size);