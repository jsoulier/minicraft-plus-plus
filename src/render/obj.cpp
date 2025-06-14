#include <SDL3/SDL.h>

#include <fast_obj.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <unordered_map>

#include <render/obj.hpp>
#include <render/util.hpp>
#include <util.hpp>

bool MppRenderObj::load(SDL_GPUDevice* device, SDL_GPUCopyPass* copyPass, const char* name)
{
    /* TODO: fix leaks on error */

    struct Vertex
    {
        uint32_t packed;
        float texCoord;

        bool operator==(const Vertex other) const
        {
            return packed == other.packed && texCoord == other.texCoord;
        }
    };

    struct Hash
    {
        size_t operator()(const Vertex vertex) const
        {
            return std::hash<uint32_t>{}(vertex.packed) ^ std::hash<float>{}(vertex.texCoord);
        }
    };


    char objPath[256]{};
    char pngPath[256]{};

    snprintf(objPath, sizeof(objPath), "%s.obj", name);
    snprintf(pngPath, sizeof(pngPath), "%s.png", name);

    fastObjMesh* mesh = fast_obj_read(objPath);
    if (!mesh)
    {
        MPP_LOG_RELEASE("Failed to load mesh: %s", objPath);
        return false;
    }

    SDL_GPUTransferBuffer* vertexTransferBuffer;
    SDL_GPUTransferBuffer* indexTransferBuffer;

    {
        SDL_GPUTransferBufferCreateInfo info{};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;

        info.size = mesh->index_count * sizeof(Vertex);
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

    Vertex* vertexData = static_cast<Vertex*>(SDL_MapGPUTransferBuffer(device, vertexTransferBuffer, false));
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

    std::unordered_map<Vertex, uint16_t, Hash> vertexToIndex;

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

        Vertex vertex{};

        int positionX = mesh->positions[positionIndex * 3 + 0] * Scale;
        int positionY = mesh->positions[positionIndex * 3 + 1] * Scale;
        int positionZ = mesh->positions[positionIndex * 3 + 2] * Scale;

        vertex.texCoord = mesh->texcoords[texCoordIndex * 2 + 0];

        int normalX = mesh->normals[normalIndex * 3 + 0];
        int normalY = mesh->normals[normalIndex * 3 + 1];
        int normalZ = mesh->normals[normalIndex * 3 + 2];

        MPP_ASSERT_RELEASE(positionX > -128 && positionX < 128);
        MPP_ASSERT_RELEASE(positionY > -128 && positionY < 128);
        MPP_ASSERT_RELEASE(positionZ > -128 && positionZ < 128);

        vertex.packed |= (abs(positionX) & 0x7F) << 0;
        vertex.packed |= (positionX < 0) << 7;
        vertex.packed |= (abs(positionY) & 0x7F) << 8;
        vertex.packed |= (positionY < 0) << 15;
        vertex.packed |= (abs(positionZ) & 0x7F) << 16;
        vertex.packed |= (positionZ < 0) << 23;

        uint32_t normal = 0;
        if (normalX > 0)
        {
            normal = 0;
        }
        else if (normalX < 0)
        {
            normal = 1;
        }
        else if (normalY > 0)
        {
            normal = 2;
        }
        else if (normalY < 0)
        {
            normal = 3;
        }
        else if (normalZ > 0)
        {
            normal = 4;
        }
        else if (normalZ < 0)
        {
            normal = 5;
        }
        else
        {
            MPP_ASSERT_RELEASE(false);
        }
        vertex.packed |= (normal & 0x7) << 24;

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
        info.size = vertexCount * sizeof(Vertex);
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
    region.size = vertexCount * sizeof(Vertex);
    SDL_UploadToGPUBuffer(copyPass, &location, &region, false);

    location.transfer_buffer = indexTransferBuffer;
    region.buffer = indexBuffer;
    region.size = indexCount * sizeof(uint16_t);
    SDL_UploadToGPUBuffer(copyPass, &location, &region, false);

    SDL_ReleaseGPUTransferBuffer(device, vertexTransferBuffer);
    SDL_ReleaseGPUTransferBuffer(device, indexTransferBuffer);

    fast_obj_destroy(mesh);

    palette = mppRenderLoadTexture(device, copyPass, pngPath);
    if (!palette)
    {
        MPP_LOG_RELEASE("Failed to load palette: %s", pngPath);
        return false;
    }

    return true;
}

void MppRenderObj::free(SDL_GPUDevice* device)
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