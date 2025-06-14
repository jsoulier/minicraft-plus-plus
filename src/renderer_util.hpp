#pragma once

#include <SDL3/SDL.h>

SDL_GPUShader* mppRendererLoadShader(SDL_GPUDevice* device, const char* name);
SDL_GPUComputePipeline* mppRendererLoadComputeShader(SDL_GPUDevice* device, const char* name);
SDL_GPUTexture* mppRendererLoadTexture(SDL_GPUDevice* device, SDL_GPUCopyPass* copyPass, const char* path);