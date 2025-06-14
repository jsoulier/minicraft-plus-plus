#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cstdint>

#include "render_util.hpp"
#include "renderer.hpp"

static SDL_Window* window;
static SDL_GPUDevice* device;
static TTF_TextEngine* textEngine;

bool mppRendererInit()
{
    window = SDL_CreateWindow("Minicraft Plus Plus", 960, 720, SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return false;
    }

#ifndef NDEBUG
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, true, nullptr);
#else
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, false, nullptr);
#endif
    if (!device)
    {
        SDL_Log("Failed to create device: %s", SDL_GetError());
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window))
    {
        SDL_Log("Failed to create swapchain: %s", SDL_GetError());
        return false;
    }

    if (!TTF_Init())
    {
        SDL_Log("Failed to initialize SDL ttf: %s", SDL_GetError());
        return false;
    }

    textEngine = TTF_CreateGPUTextEngine(device);
    if (!textEngine)
    {
        SDL_Log("Failed to create text engine: %s", SDL_GetError());
        return false;
    }

    return true;
}

void mppRendererQuit()
{
    TTF_DestroyGPUTextEngine(textEngine);
    TTF_Quit();

    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);

    SDL_DestroyWindow(window);
}

void mppRendererSubmit()
{
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (!commandBuffer)
    {
        SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
        return;
    }

    uint32_t width;
    uint32_t height;
    SDL_GPUTexture* swapchainTexture;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, &width, &height))
    {
        SDL_Log("Failed to acquire swapchain texture: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(commandBuffer);
        return;
    }

    SDL_SubmitGPUCommandBuffer(commandBuffer);
}