#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cstdint>

#include <render/buffer.hpp>
#include <render/obj.hpp>
#include <render/render.hpp>
#include <render/util.hpp>
#include <util.hpp>

static SDL_Window* window;
static SDL_GPUDevice* device;
static TTF_TextEngine* textEngine;

bool mppRenderInit()
{
    if (!(window = SDL_CreateWindow("Minicraft Plus Plus", 960, 720, SDL_WINDOW_RESIZABLE)))
    {
        MPP_LOG_RELEASE("Failed to create window: %s", SDL_GetError());
        return false;
    }

    if (!(device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, MPP_DEBUG, nullptr)))
    {
        MPP_LOG_RELEASE("Failed to create device: %s", SDL_GetError());
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window))
    {
        MPP_LOG_RELEASE("Failed to create swapchain: %s", SDL_GetError());
        return false;
    }

    if (!TTF_Init())
    {
        MPP_LOG_RELEASE("Failed to initialize SDL ttf: %s", SDL_GetError());
        return false;
    }
    
    if (!(textEngine = TTF_CreateGPUTextEngine(device)))
    {
        MPP_LOG_RELEASE("Failed to create text engine: %s", SDL_GetError());
        return false;
    }

    return true;
}

void mppRenderQuit()
{
    TTF_DestroyGPUTextEngine(textEngine);
    TTF_Quit();

    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);

    SDL_DestroyWindow(window);
}

void mppRenderSubmit()
{
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (!commandBuffer)
    {
        MPP_LOG_RELEASE("Failed to acquire command buffer: %s", SDL_GetError());
        return;
    }

    uint32_t width;
    uint32_t height;
    SDL_GPUTexture* swapchainTexture;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, &width, &height))
    {
        MPP_LOG_RELEASE("Failed to acquire swapchain texture: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(commandBuffer);
        return;
    }

    SDL_SubmitGPUCommandBuffer(commandBuffer);
}