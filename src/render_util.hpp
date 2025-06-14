#pragma once

#include <SDL3/SDL.h>

SDL_GPUShader* mppRenderLoadShader(SDL_GPUDevice* device, const char* name);
SDL_GPUComputePipeline* mppRenderLoadComputeShader(SDL_GPUDevice* device, const char* name);
SDL_GPUTexture* mppRenderLoadTexture(SDL_GPUDevice* device, SDL_GPUCopyPass* copyPass, const char* path);