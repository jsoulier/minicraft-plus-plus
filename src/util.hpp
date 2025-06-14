#pragma once

#include <SDL3/SDL.h>

#ifndef NDEBUG
#define MPP_DEBUG 1
#define MPP_RELEASE 0
#define MPP_ASSERT_DEBUG SDL_assert_always
#define MPP_ASSERT_RELEASE SDL_assert_always
#define MPP_LOG_DEBUG SDL_Log
#define MPP_LOG_RELEASE SDL_Log
#else
#define MPP_DEBUG 0
#define MPP_RELEASE 1
#define MPP_ASSERT_DEBUG
#define MPP_ASSERT_RELEASE SDL_assert_always
#define MPP_LOG_DEBUG
#define MPP_LOG_RELEASE SDL_Log
#endif