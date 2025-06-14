#include <SDL3/SDL.h>
#include <jsmn.h>
#include <stb_image.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "renderer_util.hpp"
#include "util.hpp"

SDL_GPUShader* mppRendererLoadShader(SDL_GPUDevice* device, const char* name)
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
        MPP_ASSERT_RELEASE(false);
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
        MPP_LOG_RELEASE("Failed to load shader: %s", shaderPath);
        return nullptr;
    }

    char* shaderJsonData = static_cast<char*>(SDL_LoadFile(shaderJsonPath, &shaderJsonSize));
    if (!shaderJsonData)
    {
        MPP_LOG_RELEASE("Failed to load shader json: %s", shaderJsonPath);
        return nullptr;
    }

    jsmn_parser jsonParser;
    jsmntok_t jsonTokens[9];

    jsmn_init(&jsonParser);
    if (jsmn_parse(&jsonParser, shaderJsonData, shaderJsonSize, jsonTokens, 9) <= 0)
    {
        MPP_LOG_RELEASE("Failed to parse json: %s", shaderJsonPath);
        return nullptr;
    }

    SDL_GPUShaderCreateInfo info{};

    for (int i = 1; i < 9; i += 2)
    {
        if (jsonTokens[i].type != JSMN_STRING)
        {
            MPP_LOG_RELEASE("Bad json type: %s", shaderJsonPath);
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
            MPP_ASSERT_RELEASE(false);
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
        MPP_LOG_RELEASE("Failed to create shader: %s", SDL_GetError());
        return nullptr;
    }

    SDL_free(shaderData);
    SDL_free(shaderJsonData);

    return shader;
}

SDL_GPUComputePipeline* mppRendererLoadComputeShader(SDL_GPUDevice* device, const char* name)
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
        MPP_ASSERT_RELEASE(false);
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
        MPP_LOG_RELEASE("Failed to load shader: %s", shaderPath);
        return nullptr;
    }

    char* shaderJsonData = static_cast<char*>(SDL_LoadFile(shaderJsonPath, &shaderJsonSize));
    if (!shaderJsonData)
    {
        MPP_LOG_RELEASE("Failed to load shader json: %s", shaderJsonPath);
        return nullptr;
    }

    jsmn_parser jsonParser;
    jsmntok_t jsonTokens[19];

    jsmn_init(&jsonParser);
    if (jsmn_parse(&jsonParser, shaderJsonData, shaderJsonSize, jsonTokens, 19) <= 0)
    {
        MPP_LOG_RELEASE("Failed to parse json: %s", shaderJsonPath);
        return nullptr;
    }

    SDL_GPUComputePipelineCreateInfo info{};

    for (int i = 1; i < 19; i += 2)
    {
        if (jsonTokens[i].type != JSMN_STRING)
        {
            MPP_LOG_RELEASE("Bad json type: %s", shaderJsonPath);
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
            MPP_ASSERT_RELEASE(false);
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
        MPP_LOG_RELEASE("Failed to create compute pipeline: %s", SDL_GetError());
        return nullptr;
    }
        
    SDL_free(shaderData);
    SDL_free(shaderJsonData);

    return pipeline;
}

SDL_GPUTexture* mppRendererLoadTexture(SDL_GPUDevice* device, SDL_GPUCopyPass* copyPass, const char* path)
{
    /* TODO: fix leaks on error */

    int channels;
    int width;
    int height;

    void* srcData = stbi_load(path, &width, &height, &channels, 4);
    if (!srcData)
    {
        MPP_LOG_RELEASE("Failed to load image: %s, %s", path, stbi_failure_reason());
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
            MPP_LOG_RELEASE("Failed to create transfer buffer: %s", SDL_GetError());
            return nullptr;
        }
    }

    void* dstData = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
    if (!dstData)
    {
        MPP_LOG_RELEASE("Failed to map transfer buffer: %s", SDL_GetError());
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
            MPP_LOG_RELEASE("Failed to create texture: %s", SDL_GetError());
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