#pragma once

#include <SDL3/SDL.h>

#include <cstdint>

SDL_GPUShader* mppRenderLoadShader(SDL_GPUDevice* device, const char* name);
SDL_GPUComputePipeline* mppRenderLoadComputeShader(SDL_GPUDevice* device, const char* name);
SDL_GPUTexture* mppRenderLoadTexture(SDL_GPUDevice* device, SDL_GPUCopyPass* copyPass, const char* path);

struct MppRenderMesh
{
    bool load(SDL_GPUDevice* device, SDL_GPUCopyPass* copyPass, const char* name);
    void free(SDL_GPUDevice* device);
    
    SDL_GPUTexture* palette;
    SDL_GPUBuffer* vertexBuffer;
    SDL_GPUBuffer* indexBuffer;
    uint16_t vertexCount;
    uint16_t indexCount;
};