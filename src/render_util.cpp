#include <SDL3/SDL.h>

#include <fast_obj.h>
#include <jsmn.h>
#include <stb_image.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <unordered_map>

#include "render_util.hpp"

SDL_GPUShader* mppRenderLoadShader(SDL_GPUDevice* device, const char* name)
{
    /* TODO: fix leaks on error */

    SDL_GPUShaderFormat format = SDL_GetGPUShaderFormats(device);
    const char* entrypoint;
    const char* fileExtension;

    if (format & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        entrypoint = "main";
        fileExtension = "spv";
    }
    else if (format & SDL_GPU_SHADERFORMAT_DXIL)
    {
        format = SDL_GPU_SHADERFORMAT_DXIL;
        entrypoint = "main";
        fileExtension = "dxil";
    }
    else if (format & SDL_GPU_SHADERFORMAT_MSL)
    {
        format = SDL_GPU_SHADERFORMAT_MSL;
        entrypoint = "main0";
        fileExtension = "msl";
    }
    else
    {
        SDL_assert(false);
    }

    char shaderPath[256] = {0};
    char shaderJsonPath[256] = {0};

    snprintf(shaderPath, sizeof(shaderPath), "%s.%s", name, fileExtension);
    snprintf(shaderJsonPath, sizeof(shaderJsonPath), "%s.json", name);

    size_t shaderSize;
    size_t shaderJsonSize;

    void* shaderData = SDL_LoadFile(shaderPath, &shaderSize);
    if (!shaderData)
    {
        SDL_Log("Failed to load shader: %s", shaderPath);
        return nullptr;
    }

    char* shaderJsonData = static_cast<char*>(SDL_LoadFile(shaderJsonPath, &shaderJsonSize));
    if (!shaderJsonData)
    {
        SDL_Log("Failed to load shader json: %s", shaderJsonPath);
        return nullptr;
    }

    jsmn_parser jsonParser;
    jsmntok_t jsonTokens[9];

    jsmn_init(&jsonParser);
    if (jsmn_parse(&jsonParser, shaderJsonData, shaderJsonSize, jsonTokens, 9) <= 0)
    {
        SDL_Log("Failed to parse json: %s", shaderJsonPath);
        return nullptr;
    }

    SDL_GPUShaderCreateInfo info{};

    for (int i = 1; i < 9; i += 2)
    {
        if (jsonTokens[i].type != JSMN_STRING)
        {
            SDL_Log("Bad json type: %s", shaderJsonPath);
            return nullptr;
        }

        char* keyString = shaderJsonData + jsonTokens[i + 0].start;
        char* valueString = shaderJsonData + jsonTokens[i + 1].start;
        int keySize = jsonTokens[i + 0].end - jsonTokens[i + 0].start;

        uint32_t* value;

        if (!memcmp("samplers", keyString, keySize))
        {
            value = &info.num_samplers;
        }
        else if (!memcmp("storage_textures", keyString, keySize))
        {
            value = &info.num_storage_textures;
        }
        else if (!memcmp("storage_buffers", keyString, keySize))
        {
            value = &info.num_storage_buffers;
        }
        else if (!memcmp("uniform_buffers", keyString, keySize))
        {
            value = &info.num_uniform_buffers;
        }
        else
        {
            SDL_assert(false);
        }

        *value = *valueString - '0';
    }

    info.code = static_cast<Uint8*>(shaderData);
    info.code_size = shaderSize;

    info.entrypoint = entrypoint;
    info.format = format;

    if (strstr(name, ".frag"))
    {
        info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    }
    else
    {
        info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    }

    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &info);
    if (!shader)
    {
        SDL_Log("Failed to create shader: %s", SDL_GetError());
        return nullptr;
    }

    SDL_free(shaderData);
    SDL_free(shaderJsonData);

    return shader;
}

SDL_GPUComputePipeline* mppRenderLoadComputeShader(SDL_GPUDevice* device, const char* name)
{
    /* TODO: fix leaks on error */

    SDL_GPUShaderFormat format = SDL_GetGPUShaderFormats(device);
    const char* entrypoint;
    const char* fileExtension;

    if (format & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        entrypoint = "main";
        fileExtension = "spv";
    }
    else if (format & SDL_GPU_SHADERFORMAT_DXIL)
    {
        format = SDL_GPU_SHADERFORMAT_DXIL;
        entrypoint = "main";
        fileExtension = "dxil";
    }
    else if (format & SDL_GPU_SHADERFORMAT_MSL)
    {
        format = SDL_GPU_SHADERFORMAT_MSL;
        entrypoint = "main0";
        fileExtension = "msl";
    }
    else
    {
        SDL_assert(false);
    }

    char shaderPath[256] = {0};
    char shaderJsonPath[256] = {0};

    snprintf(shaderPath, sizeof(shaderPath), "%s.%s", name, fileExtension);
    snprintf(shaderJsonPath, sizeof(shaderJsonPath), "%s.json", name);

    size_t shaderSize;
    size_t shaderJsonSize;

    void* shaderData = SDL_LoadFile(shaderPath, &shaderSize);
    if (!shaderData)
    {
        SDL_Log("Failed to load shader: %s", shaderPath);
        return nullptr;
    }

    char* shaderJsonData = static_cast<char*>(SDL_LoadFile(shaderJsonPath, &shaderJsonSize));
    if (!shaderJsonData)
    {
        SDL_Log("Failed to load shader json: %s", shaderJsonPath);
        return nullptr;
    }

    jsmn_parser jsonParser;
    jsmntok_t jsonTokens[19];

    jsmn_init(&jsonParser);
    if (jsmn_parse(&jsonParser, shaderJsonData, shaderJsonSize, jsonTokens, 19) <= 0)
    {
        SDL_Log("Failed to parse json: %s", shaderJsonPath);
        return nullptr;
    }

    SDL_GPUComputePipelineCreateInfo info{};

    for (int i = 1; i < 19; i += 2)
    {
        if (jsonTokens[i].type != JSMN_STRING)
        {
            SDL_Log("Bad json type: %s", shaderJsonPath);
            return nullptr;
        }

        char* keyString = shaderJsonData + jsonTokens[i + 0].start;
        char* valueString = shaderJsonData + jsonTokens[i + 1].start;
        int keySize = jsonTokens[i + 0].end - jsonTokens[i + 0].start;

        uint32_t* value;

        if (!memcmp("samplers", keyString, keySize))
        {
            value = &info.num_samplers;
        }
        else if (!memcmp("readonly_storage_textures", keyString, keySize))
        {
            value = &info.num_readonly_storage_textures;
        }
        else if (!memcmp("readonly_storage_buffers", keyString, keySize))
        {
            value = &info.num_readonly_storage_buffers;
        }
        else if (!memcmp("readwrite_storage_textures", keyString, keySize))
        {
            value = &info.num_readwrite_storage_textures;
        }
        else if (!memcmp("readwrite_storage_buffers", keyString, keySize))
        {
            value = &info.num_readwrite_storage_buffers;
        }
        else if (!memcmp("uniform_buffers", keyString, keySize))
        {
            value = &info.num_uniform_buffers;
        }
        else if (!memcmp("threadcount_x", keyString, keySize))
        {
            value = &info.threadcount_x;
        }
        else if (!memcmp("threadcount_y", keyString, keySize))
        {
            value = &info.threadcount_y;
        }
        else if (!memcmp("threadcount_z", keyString, keySize))
        {
            value = &info.threadcount_z;
        }
        else
        {
            SDL_assert(false);
        }

        *value = *valueString - '0';
    }

    info.code = static_cast<Uint8*>(shaderData);
    info.code_size = shaderSize;

    info.entrypoint = entrypoint;
    info.format = format;

    SDL_GPUComputePipeline* pipeline = SDL_CreateGPUComputePipeline(device, &info);
    if (!pipeline)
    {
        SDL_Log("Failed to create compute pipeline: %s", SDL_GetError());
        return nullptr;
    }
        
    SDL_free(shaderData);
    SDL_free(shaderJsonData);

    return pipeline;
}

SDL_GPUTexture* mppRenderLoadTexture(SDL_GPUDevice* device, SDL_GPUCopyPass* copyPass, const char* path)
{
    /* TODO: fix leaks on error */

    int channels;
    int width;
    int height;

    void* srcData = stbi_load(path, &width, &height, &channels, 4);
    if (!srcData)
    {
        SDL_Log("Failed to load image: %s, %s", path, stbi_failure_reason());
        return nullptr;
    }

    SDL_GPUTransferBuffer* transferBuffer;
    SDL_GPUTexture* texture;

    {
        SDL_GPUTransferBufferCreateInfo info{};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = width * height * 4;
        transferBuffer = SDL_CreateGPUTransferBuffer(device, &info);
        if (!transferBuffer)
        {
            SDL_Log("Failed to create transfer buffer: %s", SDL_GetError());
            return nullptr;
        }
    }

    void* dstData = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
    if (!dstData)
    {
        SDL_Log("Failed to map transfer buffer: %s", SDL_GetError());
        return nullptr;
    }

    memcpy(dstData, srcData, width * height * 4);
    stbi_image_free(srcData);

    SDL_UnmapGPUTransferBuffer(device, transferBuffer);

    {
        SDL_GPUTextureCreateInfo info{};

        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

        info.width = width;
        info.height = height;
        info.layer_count_or_depth = 1;

        info.num_levels = 1;

        texture = SDL_CreateGPUTexture(device, &info);
        if (!texture)
        {
            SDL_Log("Failed to create texture: %s", SDL_GetError());
            return nullptr;
        }

        SDL_DestroyProperties(info.props);
    }

    {
        SDL_GPUTextureTransferInfo info{};
        info.transfer_buffer = transferBuffer;

        SDL_GPUTextureRegion region{};
        region.texture = texture;
        region.w = width;
        region.h = height;
        region.d = 1;

        SDL_UploadToGPUTexture(copyPass, &info, &region, false);
    }

    SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

    return texture;
}

bool MppRenderMesh::load(SDL_GPUDevice* device, SDL_GPUCopyPass* copyPass, const char* name)
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
        SDL_Log("Failed to load mesh: %s", objPath);
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
            SDL_Log("Failed to create vertex transfer buffer: %s", SDL_GetError());
            return false;
        }

        info.size = mesh->index_count * sizeof(uint16_t);
        indexTransferBuffer = SDL_CreateGPUTransferBuffer(device, &info);
        if (!indexTransferBuffer)
        {
            SDL_Log("Failed to create index transfer buffer: %s", SDL_GetError());
            return false;
        }
    }

    Vertex* vertexData = static_cast<Vertex*>(SDL_MapGPUTransferBuffer(device, vertexTransferBuffer, false));
    if (!vertexData)
    {
        SDL_Log("Failed to map vertex transfer buffer: %s", SDL_GetError());
        return false;
    }

    uint16_t* indexData = static_cast<uint16_t*>(SDL_MapGPUTransferBuffer(device, indexTransferBuffer, false));
    if (!indexData)
    {
        SDL_Log("Failed to map index transfer buffer: %s", SDL_GetError());
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
            SDL_Log("Missing position data: %s", name);
            return false;
        }

        if (texCoordIndex <= 0)
        {
            SDL_Log("Missing texcoord data: %s", name);
            return false;
        }

        if (normalIndex <= 0)
        {
            SDL_Log("Missing normal data: %s", name);
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

        SDL_assert(positionX > -128 && positionX < 128);
        SDL_assert(positionY > -128 && positionY < 128);
        SDL_assert(positionZ > -128 && positionZ < 128);

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
            SDL_assert(false);
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
            SDL_Log("Failed to create vertex buffer: %s", SDL_GetError());
            return false;
        }

        info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        info.size = indexCount * sizeof(uint16_t);
        indexBuffer = SDL_CreateGPUBuffer(device, &info);
        if (!indexBuffer)
        {
            SDL_Log("Failed to create index buffer: %s", SDL_GetError());
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
        SDL_Log("Failed to load palette: %s", pngPath);
        return false;
    }

    return true;
}

void MppRenderMesh::free(SDL_GPUDevice* device)
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