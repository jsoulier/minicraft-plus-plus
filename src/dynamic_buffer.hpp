#pragma once

#include <SDL3/SDL.h>

#include <cstdint>

struct MppDynamicBufferBase
{

};

template<typename T>
struct MppDynamicBuffer : public MppDynamicBufferBase
{

};