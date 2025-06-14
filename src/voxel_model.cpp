#include <SDL3/SDL.h>
#include <fast_obj.h>

#include <cstdint>
#include <cstdio>
#include <unordered_map>

#include "renderer_util.hpp"
#include "util.hpp"
#include "voxel.hpp"
#include "voxel_model.hpp"

bool MppVoxelModel::load(SDL_GPUDevice* device, SDL_GPUCopyPass* copyPass, const char* name)
{
    /* TODO: fix leaks on error */

    char modelPath[256]{};
    char pngPath[256]{};

    snprintf(modelPath, sizeof(modelPath), "%s.model", name);
    snprintf(pngPath, sizeof(pngPath), "%s.png", name);

    fastObjMesh* mesh = fast_obj_read(modelPath);
    if (!mesh)
    {
        MPP_LOG_RELEASE("Failed to load model: %s", modelPath);
        return false;
    }

    SDL_GPUTransferBuffer* vertexTransferBuffer;
    SDL_GPUTransferBuffer* indexTransferBuffer;

    {
        SDL_GPUTransferBufferCreateInfo info{};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;

        info.size = mesh->index_count * sizeof(MppVoxel);
        vertexTransferBuffer = SDL_CreateGPUTransferBuffer(device, &info);
        if (!vertexTransferBuffer)
        {
            MPP_LOG_RELEASE("Failed to create vertex transfer buffer: %s", SDL_GetError());
            return false;
        }

        info.size = mesh->index_count * sizeof(uint16_t);
        indexTransferBuffer = SDL_CreateGPUTransferBuffer(device, &info);
        if (!indexTransferBuffer)
        {
            MPP_LOG_RELEASE("Failed to create index transfer buffer: %s", SDL_GetError());
            return false;
        }
    }

    MppVoxel* vertexData = static_cast<MppVoxel*>(SDL_MapGPUTransferBuffer(device, vertexTransferBuffer, false));
    if (!vertexData)
    {
        MPP_LOG_RELEASE("Failed to map vertex transfer buffer: %s", SDL_GetError());
        return false;
    }

    uint16_t* indexData = static_cast<uint16_t*>(SDL_MapGPUTransferBuffer(device, indexTransferBuffer, false));
    if (!indexData)
    {
        MPP_LOG_RELEASE("Failed to map index transfer buffer: %s", SDL_GetError());
        return false;
    }

    std::unordered_map<MppVoxel, uint16_t> vertexToIndex;

    vertexCount = 0;
    indexCount = mesh->index_count;

    for (uint32_t i = 0; i < mesh->index_count; i++)
    {
        uint32_t positionIndex = mesh->indices[i].p;
        uint32_t texCoordIndex = mesh->indices[i].t;
        uint32_t normalIndex = mesh->indices[i].n;

        if (positionIndex <= 0)
        {
            MPP_LOG_RELEASE("Missing position data: %s", name);
            return false;
        }

        if (texCoordIndex <= 0)
        {
            MPP_LOG_RELEASE("Missing texcoord data: %s", name);
            return false;
        }

        if (normalIndex <= 0)
        {
            MPP_LOG_RELEASE("Missing normal data: %s", name);
            return false;
        }

        static constexpr float Scale = 10.0f;

        MppVoxel vertex(
            mesh->positions[positionIndex * 3 + 0] * Scale,
            mesh->positions[positionIndex * 3 + 1] * Scale,
            mesh->positions[positionIndex * 3 + 2] * Scale,
            mesh->normals[normalIndex * 3 + 0],
            mesh->normals[normalIndex * 3 + 1],
            mesh->normals[normalIndex * 3 + 2],
            mesh->texcoords[texCoordIndex * 2 + 0]);

        auto it = vertexToIndex.find(vertex);
        if (it == vertexToIndex.end())
        {
            vertexToIndex[vertex] = vertexCount;
            vertexData[vertexCount] = vertex;
            indexData[i] = vertexCount++;
        }
        else
        {
            indexData[i] = it->second;
        }
    }

    SDL_UnmapGPUTransferBuffer(device, vertexTransferBuffer);
    SDL_UnmapGPUTransferBuffer(device, indexTransferBuffer);

    {
        SDL_GPUBufferCreateInfo info = {0};

        info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        info.size = vertexCount * sizeof(MppVoxel);
        vertexBuffer = SDL_CreateGPUBuffer(device, &info);
        if (!vertexBuffer)
        {
            MPP_LOG_RELEASE("Failed to create vertex buffer: %s", SDL_GetError());
            return false;
        }

        info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        info.size = indexCount * sizeof(uint16_t);
        indexBuffer = SDL_CreateGPUBuffer(device, &info);
        if (!indexBuffer)
        {
            MPP_LOG_RELEASE("Failed to create index buffer: %s", SDL_GetError());
            return false;
        }    
    }

    SDL_GPUTransferBufferLocation location = {0};
    SDL_GPUBufferRegion region = {0};

    location.transfer_buffer = vertexTransferBuffer;
    region.buffer = vertexBuffer;
    region.size = vertexCount * sizeof(MppVoxel);
    SDL_UploadToGPUBuffer(copyPass, &location, &region, false);

    location.transfer_buffer = indexTransferBuffer;
    region.buffer = indexBuffer;
    region.size = indexCount * sizeof(uint16_t);
    SDL_UploadToGPUBuffer(copyPass, &location, &region, false);

    SDL_ReleaseGPUTransferBuffer(device, vertexTransferBuffer);
    SDL_ReleaseGPUTransferBuffer(device, indexTransferBuffer);

    fast_obj_destroy(mesh);

    palette = mppRendererLoadTexture(device, copyPass, pngPath);
    if (!palette)
    {
        MPP_LOG_RELEASE("Failed to load palette: %s", pngPath);
        return false;
    }

    return true;
}

void MppVoxelModel::free(SDL_GPUDevice* device)
{
    if (palette)
    {
        SDL_ReleaseGPUTexture(device, palette);
        palette = nullptr;
    }

    if (vertexBuffer)
    {
        SDL_ReleaseGPUBuffer(device, vertexBuffer);
        vertexBuffer = nullptr;
    }

    if (indexBuffer)
    {
        SDL_ReleaseGPUBuffer(device, indexBuffer);
        indexBuffer = nullptr;
    }
}