#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <fast_obj.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <stb_image.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <fstream>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "window.hpp"

static constexpr const char* Title = "Minicraft++";
static constexpr int WindowWidth = 960;
static constexpr int WindowHeight = 720;

static SDL_Window* window;
static SDL_GPUDevice* device;
static TTF_TextEngine* textEngine;

struct ShaderConfig
{
    SDL_GPUShaderFormat format;
    std::string_view suffix;
    std::string_view entrypoint;
    int numUniformBuffers;
    int numSamplers;
    int numStorageBuffers;
    int numStorageTextures;
    int numReadWriteStorageBuffers;
    int numReadOnlyStorageBuffers;
    int numReadWriteStorageTextures;
    int numReadOnlyStorageTextures;
    int threadCountX;
    int threadCountY;
    int threadCountZ;
};

static std::optional<ShaderConfig> loadShaderConfig(const std::string_view& name)
{
    std::string path = std::format("{}.json", name);
    std::ifstream file(path);
    if (!file)
    {
        SDL_Log("Failed to open file: %s", name.data());
        return {};
    }

    nlohmann::json json;
    try
    {
        file >> json;
    }
    catch (const nlohmann::json::parse_error& e)
    {
        SDL_Log("Failed to parse json: %s, %s", e.what(), name.data());
        return {};
    }

    ShaderConfig config;
    config.format = SDL_GetGPUShaderFormats(device);
    if (config.format & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        config.format = SDL_GPU_SHADERFORMAT_SPIRV;
        config.entrypoint = "main";
        config.suffix = "spv";
    }
    else if (config.format & SDL_GPU_SHADERFORMAT_MSL)
    {
        config.format = SDL_GPU_SHADERFORMAT_MSL;
        config.entrypoint = "main0";
        config.suffix = "msl";
    }
    else
    {
        SDL_assert(false);
        return {};
    }

    config.numUniformBuffers = json.value("uniform_buffers", 0);
    config.numSamplers = json.value("samplers", 0);
    config.numStorageBuffers = json.value("storage_buffers", 0);
    config.numStorageTextures = json.value("storage_textures", 0);
    config.numReadWriteStorageBuffers = json.value("readwrite_storage_buffers", 0);
    config.numReadOnlyStorageBuffers = json.value("readonly_storage_buffers", 0);
    config.numReadWriteStorageTextures = json.value("readwrite_storage_textures", 0);
    config.numReadOnlyStorageTextures = json.value("readonly_storage_textures", 0);
    config.threadCountX = json.value("threadcount_x", 0);
    config.threadCountY = json.value("threadcount_y", 0);
    config.threadCountZ = json.value("threadcount_z", 0);

    return config;
}

static std::vector<Uint8> loadShaderData(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        SDL_Log("Failed to open file: %s", path.data());
        return {};
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<Uint8> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);

    return data;
}

static SDL_GPUShader* loadShader(const std::string_view& name)
{
    std::optional<ShaderConfig> config = loadShaderConfig(name);
    if (!config)
    {
        SDL_Log("Failed to load shader config: %s", name.data());
        return nullptr;
    }

    std::vector<Uint8> data = loadShaderData(std::format("{}.{}", name, config->suffix));
    if (data.empty())
    {
        SDL_Log("Failed to load shader data: %s", name.data());
        return nullptr;
    }

    SDL_GPUShaderCreateInfo info{};
    info.code = data.data();
    info.code_size = data.size();
    info.format = config->format;
    info.entrypoint = config->entrypoint.data();
    info.num_uniform_buffers = config->numUniformBuffers;
    info.num_samplers = config->numSamplers;
    info.num_storage_buffers = config->numStorageBuffers;
    info.num_storage_textures = config->numStorageTextures;
    if (name.contains(".frag"))
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
        SDL_Log("Failed to create shader: %s, %s", SDL_GetError(), name.data());
        return nullptr;
    }

    return shader;
}

static SDL_GPUComputePipeline* loadComputeShader(const std::string_view& name)
{
    std::optional<ShaderConfig> config = loadShaderConfig(name);
    if (!config)
    {
        SDL_Log("Failed to load shader config: %s", name.data());
        return nullptr;
    }

    std::vector<Uint8> data = loadShaderData(std::format("{}.{}", name, config->suffix));
    if (data.empty())
    {
        SDL_Log("Failed to load shader data: %s", name.data());
        return nullptr;
    }

    SDL_GPUComputePipelineCreateInfo info{};
    info.code = data.data();
    info.code_size = data.size();
    info.format = config->format;
    info.entrypoint = config->entrypoint.data();
    info.num_uniform_buffers = config->numUniformBuffers;
    info.num_samplers = config->numSamplers;
    info.num_readwrite_storage_buffers = config->numReadWriteStorageBuffers;
    info.num_readonly_storage_buffers = config->numReadOnlyStorageBuffers;
    info.num_readwrite_storage_textures = config->numReadWriteStorageTextures;
    info.num_readonly_storage_textures = config->numReadOnlyStorageTextures;

    SDL_GPUComputePipeline* shader = SDL_CreateGPUComputePipeline(device, &info);
    if (!shader)
    {
        SDL_Log("Failed to create compute pipeline: %s, %s", SDL_GetError(), name.data());
        return nullptr;
    }

    return shader;
}

static SDL_GPUTexture* loadTexture(SDL_GPUCopyPass* copyPass, const std::string_view& path)
{
    int channels;
    int width;
    int height;
    void* srcData = stbi_load(path.data(), &width, &height, &channels, 4);
    if (!srcData)
    {
        SDL_Log("Failed to load image: %s, %s", path.data(), stbi_failure_reason());
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

    std::memcpy(dstData, srcData, width * height * 4);
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
    }

    SDL_GPUTextureTransferInfo info{};
    info.transfer_buffer = transferBuffer;
    SDL_GPUTextureRegion region{};
    region.texture = texture;
    region.w = width;
    region.h = height;
    region.d = 1;
    SDL_UploadToGPUTexture(copyPass, &info, &region, false);
    SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

    return texture;
}

struct Voxel
{
    /*
     * 00-06: x magnitude
     * 07-07: x direction (1 for negative)
     * 08-14: y amplitude
     * 15-15: y direction (1 for negative)
     * 16-22: z amplitude
     * 23-23: z direction (1 for negative)
     * 24-26: normal
     * 27-31: unused
     */
    uint32_t packed;
    float texcoord;

    bool operator==(const Voxel other) const
    {
        return packed == other.packed && texcoord == other.texcoord;
    }
};

namespace std
{
    template<>
    struct hash<Voxel>
    {
        size_t operator()(const Voxel voxel) const
        {
            return std::hash<uint32_t>{}(voxel.packed) ^ std::hash<float>{}(voxel.texcoord);
        }
    };
}

struct Model
{
    SDL_GPUBuffer* vertexBuffer;
    SDL_GPUBuffer* indexBuffer;
    SDL_GPUTexture* paletteTexture;
    uint32_t vertexCount;
    uint32_t indexCount;

    bool load(SDL_GPUCopyPass* copyPass, const std::string_view& name)
    {
        std::string objPath = std::format("{}.obj", name);
        std::string pngPath = std::format("{}.png", name);

        fastObjMesh* objMesh = fast_obj_read(objPath.data());
        if (!objMesh)
        {
            SDL_Log("Failed to load mesh: %s", objPath.data());
            return false;
        }

        SDL_GPUTransferBuffer* vertexTransferBuffer;
        SDL_GPUTransferBuffer* indexTransferBuffer;

        {
            SDL_GPUTransferBufferCreateInfo info{};
            info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            info.size = objMesh->index_count * sizeof(Voxel);
            vertexTransferBuffer = SDL_CreateGPUTransferBuffer(device, &info);
            if (!vertexTransferBuffer)
            {
                SDL_Log("Failed to create vertex transfer buffer: %s", SDL_GetError());
                return false;
            }

            info.size = objMesh->index_count * sizeof(uint16_t);
            indexTransferBuffer = SDL_CreateGPUTransferBuffer(device, &info);
            if (!indexTransferBuffer)
            {
                SDL_Log("Failed to create index transfer buffer: %s", SDL_GetError());
                return false;
            }
        }

        Voxel* vertexData = static_cast<Voxel*>(SDL_MapGPUTransferBuffer(device, vertexTransferBuffer, false));
        uint16_t* indexData = static_cast<uint16_t*>(SDL_MapGPUTransferBuffer(device, indexTransferBuffer, false));
        if (!vertexData || !indexData)
        {
            SDL_Log("Failed to map transfer buffer(s): %s", SDL_GetError());
            return false;
        }

        std::unordered_map<Voxel, uint16_t> vertexToIndex;
        vertexCount = 0;
        indexCount = objMesh->index_count;
        for (uint32_t i = 0; i < indexCount; i++)
        {
            uint32_t positionIndex = objMesh->indices[i].p;
            if (positionIndex <= 0)
            {
                SDL_Log("Missing position data: %s", name);
                return false;
            }

            uint32_t normalIndex = objMesh->indices[i].n;
            if (normalIndex <= 0)
            {
                SDL_Log("Missing normal data: %s", name);
                return false;
            }

            uint32_t texcoordIndex = objMesh->indices[i].t;
            if (texcoordIndex <= 0)
            {
                SDL_Log("Missing texcoord data: %s", name);
                return false;
            }

            int positionX = objMesh->positions[positionIndex * 3 + 0] * 10.0f;
            int positionY = objMesh->positions[positionIndex * 3 + 1] * 10.0f;
            int positionZ = objMesh->positions[positionIndex * 3 + 2] * 10.0f;
            SDL_assert(positionX > -128 && positionX < 128);
            SDL_assert(positionY > -128 && positionY < 128);
            SDL_assert(positionZ > -128 && positionZ < 128);
            int normalX = objMesh->normals[normalIndex * 3 + 0];
            int normalY = objMesh->normals[normalIndex * 3 + 1];
            int normalZ = objMesh->normals[normalIndex * 3 + 2];

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

            Voxel vertex{};
            vertex.packed |= (std::abs(positionX) & 0x7F) << 0;
            vertex.packed |= (positionX < 0) << 7;
            vertex.packed |= (std::abs(positionY) & 0x7F) << 8;
            vertex.packed |= (positionY < 0) << 15;
            vertex.packed |= (std::abs(positionZ) & 0x7F) << 16;
            vertex.packed |= (positionZ < 0) << 23;
            vertex.packed |= (normal & 0x7) << 24;
            vertex.texcoord = objMesh->texcoords[texcoordIndex * 2 + 0];

            auto it = vertexToIndex.find(vertex);
            if (it == vertexToIndex.end())
            {
                vertexToIndex.emplace(vertex, vertexCount);
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
            SDL_GPUBufferCreateInfo info{};
            info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
            info.size = vertexCount * sizeof(Voxel);
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

        SDL_GPUTransferBufferLocation location{};
        SDL_GPUBufferRegion region{};
        location.transfer_buffer = vertexTransferBuffer;
        region.buffer = vertexBuffer;
        region.size = vertexCount * sizeof(Voxel);
        SDL_UploadToGPUBuffer(copyPass, &location, &region, false);
        location.transfer_buffer = indexTransferBuffer;
        region.buffer = indexBuffer;
        region.size = indexCount * sizeof(uint16_t);
        SDL_UploadToGPUBuffer(copyPass, &location, &region, false);
        SDL_ReleaseGPUTransferBuffer(device, vertexTransferBuffer);
        SDL_ReleaseGPUTransferBuffer(device, indexTransferBuffer);

        paletteTexture = loadTexture(copyPass, pngPath);
        if (!paletteTexture)
        {
            SDL_Log("Failed to load palette: %s", pngPath.data());
            return false;
        }

        fast_obj_destroy(objMesh);

        return true; 
    }
};

static bool createDevice()
{
    SDL_PropertiesID properties = SDL_CreateProperties();

#if SDL_PLATFORM_APPLE
    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.shaders.msl", true);
#else
    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.shaders.spirv", true);
#endif
#ifndef NDEBUG
    bool debug = true;
#else
    bool debug = false;
#endif

    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.debugmode", debug);
    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.verbose", debug);
    SDL_SetBooleanProperty(properties, "SDL.gpu.device.create.preferlowpower", true);

    device = SDL_CreateGPUDeviceWithProperties(properties);
    if (!device)
    {
        SDL_Log("Failed to create device: %s", SDL_GetError());
        return false;
    }

    SDL_DestroyProperties(properties);

    return true;
}

bool windowInit()
{
    SDL_SetAppMetadata(Title, nullptr, nullptr);

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow(Title, WindowWidth, WindowHeight, SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return false;
    }

    if (!createDevice())
    {
        SDL_Log("Failed to create device");
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window))
    {
        SDL_Log("Failed to claim window: %s", SDL_GetError());
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

void windowQuit()
{
    TTF_DestroyGPUTextEngine(textEngine);
    TTF_Quit();
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();

    textEngine = nullptr;
    device = nullptr;
    window = nullptr;
}