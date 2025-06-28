#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <fast_obj.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <nlohmann/json.hpp>
#include <stb_image.h>

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <format>
#include <fstream>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "renderer.hpp"

enum Shader
{
    ShaderModelPOVFrag,
    ShaderModelVert,
    ShaderCount,
};

enum GraphicsPipeline
{
    GraphicsPipelineModelPOV,
    GraphicsPipelineCount,
};

enum ComputePipeline
{
    ComputePipelineReadback,
    ComputePipelineCount,
};

enum RenderTarget
{
    RenderTargetPositionPOV,
    RenderTargetColorPOV,
    RenderTargetDepthPOV,
    RenderTargetCount,
};

enum Sampler
{
    SamplerLinearClamp,
    SamplerNearestClamp,
    SamplerCount,
};

static constexpr float Epsilon = std::numeric_limits<float>::epsilon();
static constexpr const char* Title = "Minicraft++";
static constexpr int WindowWidth = 960;
static constexpr int WindowHeight = 720;
static constexpr float POVSpeed = 1.0f;
static constexpr float POVPitch = glm::radians(-45.0f);
static constexpr float POVDistance = 100.0f;
static constexpr float POVFOV = glm::radians(60.0f);
static constexpr float POVNear = 0.1f;
static constexpr float POVFar = 1000.0f;
static constexpr glm::vec3 POVUp{0.0f, 1.0f, 0.0f};

static constexpr std::string_view Shaders[] =
{
    "model_pov.frag",
    "model.vert",
};

static constexpr std::string_view ComputePipelines[] =
{
    "readback.comp",
};

static constexpr std::string_view Models[] =
{
    "grass",
};

static_assert(SDL_arraysize(Shaders) == ShaderCount);
static_assert(SDL_arraysize(Models) == RendererModelCount);
static_assert(SDL_arraysize(ComputePipelines) == ComputePipelineCount);

static SDL_Window* window;
static SDL_GPUDevice* device;
static TTF_TextEngine* textEngine;

struct ShaderConfig
{
    SDL_GPUShaderFormat format;
    std::string_view suffix;
    std::string_view entrypoint;
    std::vector<Uint8> data;
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

static std::optional<ShaderConfig> LoadShaderConfig(const std::string_view& name)
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
    catch (const std::exception& e)
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
        assert(false);
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

static std::vector<Uint8> LoadShaderData(const std::string& path)
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

static SDL_GPUShader* LoadShader(const std::string_view& name)
{
    std::optional<ShaderConfig> config = LoadShaderConfig(name);
    if (!config)
    {
        SDL_Log("Failed to load shader config: %s", name.data());
        return nullptr;
    }

    const std::vector<Uint8>& data = LoadShaderData(std::format("{}.{}", name, config->suffix));
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
        SDL_Log("Failed to create shader: %s, %s", name.data(), SDL_GetError());
        return nullptr;
    }

    return shader;
}

static SDL_GPUComputePipeline* LoadComputePipeline(const std::string_view& name)
{
    std::optional<ShaderConfig> config = LoadShaderConfig(name);
    if (!config)
    {
        SDL_Log("Failed to load shader config: %s", name.data());
        return nullptr;
    }

    const std::vector<Uint8>& data = LoadShaderData(std::format("{}.{}", name, config->suffix));
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
    info.threadcount_x = config->threadCountX;
    info.threadcount_y = config->threadCountY;
    info.threadcount_z = config->threadCountZ;

    SDL_GPUComputePipeline* shader = SDL_CreateGPUComputePipeline(device, &info);
    if (!shader)
    {
        SDL_Log("Failed to create compute pipeline: %s, %s", name.data(), SDL_GetError());
        return nullptr;
    }

    return shader;
}

static SDL_GPUTexture* LoadTexture(SDL_GPUCopyPass* copyPass, const std::string_view& path)
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
        return packed == other.packed && std::abs(texcoord - other.texcoord) < Epsilon;
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

    bool Load(SDL_GPUCopyPass* copyPass, const std::string_view& name)
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
            assert(positionX > -128 && positionX < 128);
            assert(positionY > -128 && positionY < 128);
            assert(positionZ > -128 && positionZ < 128);
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
                assert(false);
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

        paletteTexture = LoadTexture(copyPass, pngPath);
        if (!paletteTexture)
        {
            SDL_Log("Failed to load palette: %s", pngPath.data());
            return false;
        }

        fast_obj_destroy(objMesh);

        return true; 
    }

    void Destroy()
    {
        SDL_ReleaseGPUBuffer(device, vertexBuffer);
        SDL_ReleaseGPUBuffer(device, indexBuffer);
        SDL_ReleaseGPUTexture(device, paletteTexture);

        vertexBuffer = nullptr;
        indexBuffer = nullptr;
        paletteTexture = nullptr;
    }
};

template<typename T, SDL_GPUBufferUsageFlags U = SDL_GPU_BUFFERUSAGE_VERTEX>
struct Buffer
{
    SDL_GPUBuffer* buffer;
    SDL_GPUTransferBuffer* transferBuffer;
    uint32_t size;
    uint32_t capacity;
    T* data;
    bool resize;

    template<typename... Args>
    void Emplace(Args&&... args)
    {
        if (!data && transferBuffer)
        {
            size = 0;
            resize = false;
            data = static_cast<T*>(SDL_MapGPUTransferBuffer(device, transferBuffer, true));
            if (!data)
            {
                SDL_Log("Failed to map transfer buffer: %s", SDL_GetError());
                return;
            }
        }

        if (size == capacity)
        {
            uint32_t newCapacity;
            if (size)
            {
                newCapacity = size * 2;
            }
            else
            {
                newCapacity = 10;
            }

            SDL_GPUTransferBufferCreateInfo info{};
            info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            info.size = newCapacity * sizeof(T);
            SDL_GPUTransferBuffer* newTransferBuffer = SDL_CreateGPUTransferBuffer(device, &info);
            if (!newTransferBuffer)
            {
                SDL_Log("Failed to create transfer buffer: %s", SDL_GetError());
                return;
            }

            T* newData = static_cast<T*>(SDL_MapGPUTransferBuffer(device, newTransferBuffer, false));
            if (!newData)
            {
                SDL_Log("Failed to map transfer buffer: %s", SDL_GetError());
                SDL_ReleaseGPUTransferBuffer(device, newTransferBuffer);
                return;
            }

            if (data)
            {
                std::memcpy(newData, data, size * sizeof(T));
                SDL_UnmapGPUTransferBuffer(device, transferBuffer);
            }

            SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
            capacity = newCapacity;
            transferBuffer = newTransferBuffer;
            data = newData;
            resize = true;
        }

        data[size++] = T{std::forward<Args>(args)...};
    }

    void Upload(SDL_GPUCopyPass* copyPass)
    {
        if (resize)
        {
            SDL_ReleaseGPUBuffer(device, buffer);
            buffer = nullptr;
            SDL_GPUBufferCreateInfo info{};
            info.usage = U;
            info.size = capacity * sizeof(T);
            buffer = SDL_CreateGPUBuffer(device, &info);
            if (!buffer)
            {
                SDL_Log("Failed to create buffer: %s", SDL_GetError());
                return;
            }

            resize = false;
        }

        if (data)
        {
            SDL_UnmapGPUTransferBuffer(device, transferBuffer);
            data = nullptr;
        }

        if (size)
        {
            SDL_GPUTransferBufferLocation location{};
            SDL_GPUBufferRegion region{};
            location.transfer_buffer = transferBuffer;
            region.buffer = buffer;
            region.size = size * sizeof(T);
            SDL_UploadToGPUBuffer(copyPass, &location, &region, true);
        }
    }

    void Destroy()
    {
        SDL_ReleaseGPUBuffer(device, buffer);
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

        buffer = nullptr;
        transferBuffer = nullptr;
    }
};

struct Transform
{
    glm::vec3 position;
    float rotation;
};

static SDL_GPUTextureFormat swapchainTextureFormat;
static SDL_GPUTextureFormat depthTextureFormat;
static std::array<SDL_GPUShader*, ShaderCount> shaders;
static std::array<SDL_GPUGraphicsPipeline*, GraphicsPipelineCount> graphicsPipelines;
static std::array<SDL_GPUComputePipeline*, ComputePipelineCount> computePipelines;
static std::array<SDL_GPUTexture*, RenderTargetCount> renderTargets;
static std::array<SDL_GPUSampler*, SamplerCount> samplers;
static std::array<Model, RendererModelCount> models;
static std::array<Buffer<Transform>, RendererModelCount> modelInstances;
static int swapchainWidth;
static int swapchainHeight;
static int renderTargetWidth;
static int renderTargetHeight;
static glm::mat4 povViewProjMatrix;
static glm::vec3 povPosition;
static float povRotation;

static bool CreateDevice()
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

static bool LoadModels()
{
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (!commandBuffer)
    {
        SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
        return false;
    }

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
    if (!copyPass)
    {
        SDL_Log("Failed to begin copy pass: %s", SDL_GetError());
        return false;
    }

    for (int i = 0; i < RendererModelCount; i++)
    {
        if (!models[i].Load(copyPass, Models[i]))
        {
            SDL_Log("Failed to load model: %d", i);
            return false;
        }
    }

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);

    return true;
}

static bool CreateSamplers()
{
    SDL_GPUSamplerCreateInfo info{};
    info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.min_filter = SDL_GPU_FILTER_LINEAR;
    info.mag_filter = SDL_GPU_FILTER_LINEAR;
    info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    samplers[SamplerLinearClamp] = SDL_CreateGPUSampler(device, &info);
    if (!samplers[SamplerLinearClamp])
    {
        SDL_Log("Failed to create linear clamp sampler: %s", SDL_GetError());
        return false;
    }

    info.min_filter = SDL_GPU_FILTER_NEAREST;
    info.mag_filter = SDL_GPU_FILTER_NEAREST;
    info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    samplers[SamplerNearestClamp] = SDL_CreateGPUSampler(device, &info);
    if (!samplers[SamplerNearestClamp])
    {
        SDL_Log("Failed to create nearest clamp sampler: %s", SDL_GetError());
        return false;
    }

    return true;
}

static bool ResizeRenderTargets(int width, int height)
{
    if (width == swapchainWidth && height == swapchainHeight)
    {
        return true;
    }

    for (int i = 0; i < RenderTargetCount; i++)
    {
        SDL_ReleaseGPUTexture(device, renderTargets[i]);
        renderTargets[i] = nullptr;
    }

    renderTargetWidth = width;
    renderTargetHeight = height;

    SDL_GPUTextureCreateInfo info{};
    info.width = renderTargetWidth;
    info.height = renderTargetHeight;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT;
    info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ;
    renderTargets[RenderTargetPositionPOV] = SDL_CreateGPUTexture(device, &info);
    if (!renderTargets[RenderTargetPositionPOV])
    {
        SDL_Log("Failed to create render target: %s", SDL_GetError());
        return false;
    }

    info.format = swapchainTextureFormat;
    info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    renderTargets[RenderTargetColorPOV] = SDL_CreateGPUTexture(device, &info);
    if (!renderTargets[RenderTargetColorPOV])
    {
        SDL_Log("Failed to create render target: %s", SDL_GetError());
        return false;
    }

    info.format = depthTextureFormat;
    info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    renderTargets[RenderTargetDepthPOV] = SDL_CreateGPUTexture(device, &info);
    if (!renderTargets[RenderTargetDepthPOV])
    {
        SDL_Log("Failed to create render target: %s", SDL_GetError());
        return false;
    }

    swapchainWidth = width;
    swapchainHeight = height;

    return true;
}

static void CreateModelPOVGraphicsPipeline()
{
    SDL_GPUVertexAttribute attribs[4]{};
    SDL_GPUVertexBufferDescription buffers[2]{};
    SDL_GPUColorTargetDescription targets[2]{};
    attribs[0].location = 0;
    attribs[0].buffer_slot = 0;
    attribs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_UINT;
    attribs[0].offset = offsetof(Voxel, packed);
    attribs[1].location = 1;
    attribs[1].buffer_slot = 0;
    attribs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
    attribs[1].offset = offsetof(Voxel, texcoord);
    attribs[2].location = 2;
    attribs[2].buffer_slot = 1;
    attribs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    attribs[2].offset = offsetof(Transform, position);
    attribs[3].location = 3;
    attribs[3].buffer_slot = 1;
    attribs[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
    attribs[3].offset = offsetof(Transform, rotation);
    buffers[0].slot = 0;
    buffers[0].pitch = sizeof(Voxel);
    buffers[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    buffers[0].instance_step_rate = 0;
    buffers[1].slot = 1;
    buffers[1].pitch = sizeof(Transform);
    buffers[1].input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;
    buffers[1].instance_step_rate = 0;
    targets[0].format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT;
    targets[1].format = swapchainTextureFormat;

    SDL_GPUGraphicsPipelineCreateInfo info{};
    info.vertex_shader = shaders[ShaderModelVert];
    info.fragment_shader = shaders[ShaderModelPOVFrag];
    info.vertex_input_state.vertex_buffer_descriptions = buffers;
    info.vertex_input_state.num_vertex_buffers = 2;
    info.vertex_input_state.vertex_attributes = attribs;
    info.vertex_input_state.num_vertex_attributes = 4;
    info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
    info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
    info.depth_stencil_state.enable_depth_test = true;
    info.depth_stencil_state.enable_depth_write = true;
    info.target_info.color_target_descriptions = targets;
    info.target_info.num_color_targets = 2;
    info.target_info.depth_stencil_format = depthTextureFormat;
    info.target_info.has_depth_stencil_target = true;

    graphicsPipelines[GraphicsPipelineModelPOV] = SDL_CreateGPUGraphicsPipeline(device, &info);
}

static bool CreateGraphicsPipelines()
{
    CreateModelPOVGraphicsPipeline();

    for (int i = 0; i < GraphicsPipelineCount; i++)
    {
        if (!graphicsPipelines[i])
        {
            SDL_Log("Failed to create graphics pipeline: %d, %s", i, SDL_GetError());
            return false;
        }
    }

    return true;
}

bool RendererInit()
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

    if (!CreateDevice())
    {
        SDL_Log("Failed to create device");
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window))
    {
        SDL_Log("Failed to claim window: %s", SDL_GetError());
        return false;
    }

    swapchainTextureFormat = SDL_GetGPUSwapchainTextureFormat(device, window);
    if (SDL_GPUTextureSupportsFormat(device, SDL_GPU_TEXTUREFORMAT_D32_FLOAT, SDL_GPU_TEXTURETYPE_2D,
        SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER))
    {
        depthTextureFormat = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    }
    else
    {
        depthTextureFormat = SDL_GPU_TEXTUREFORMAT_D24_UNORM;
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

    for (int i = 0; i < ShaderCount; i++)
    {
        shaders[i] = LoadShader(Shaders[i]);
        if (!shaders[i])
        {
            SDL_Log("Failed to load shader: %d", i);
            return false;
        }
    }

    for (int i = 0; i < ComputePipelineCount; i++)
    {
        computePipelines[i] = LoadComputePipeline(ComputePipelines[i]);
        if (!computePipelines[i])
        {
            SDL_Log("Failed to load compute pipeline: %d", i);
            return false;
        }
    }

    if (!LoadModels())
    {
        SDL_Log("Failed to load models");
        return false;
    }

    if (!CreateSamplers())
    {
        SDL_Log("Failed to create samplers");
        return false;
    }

    if (!CreateGraphicsPipelines())
    {
        SDL_Log("Failed to create graphics pipelines");
        return false;
    }

    return true;
}

void RendererQuit()
{
    for (int i = 0; i < ShaderCount; i++)
    {
        SDL_ReleaseGPUShader(device, shaders[i]);
        shaders[i] = nullptr;
    }

    for (int i = 0; i < GraphicsPipelineCount; i++)
    {
        SDL_ReleaseGPUGraphicsPipeline(device, graphicsPipelines[i]);
        graphicsPipelines[i] = nullptr;
    }

    for (int i = 0; i < ComputePipelineCount; i++)
    {
        SDL_ReleaseGPUComputePipeline(device, computePipelines[i]);
        computePipelines[i] = nullptr;
    }

    for (int i = 0; i < SamplerCount; i++)
    {
        SDL_ReleaseGPUSampler(device, samplers[i]);
        samplers[i] = nullptr;
    }

    for (int i = 0; i < RenderTargetCount; i++)
    {
        SDL_ReleaseGPUTexture(device, renderTargets[i]);
        renderTargets[i] = nullptr;
    }

    for (int i = 0; i < RendererModelCount; i++)
    {
        models[i].Destroy();
        modelInstances[i].Destroy();
    }

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

void RendererMove(const glm::vec3& position, float rotation)
{
    povViewProjMatrix = {};
    if (renderTargetWidth < Epsilon || renderTargetHeight < Epsilon)
    {
        return;
    }

    povPosition = glm::mix(povPosition, position, POVSpeed);
    povRotation = glm::mix(povRotation, rotation, POVSpeed);

    glm::vec3 povForward;
    povForward.x = std::cos(POVPitch) * std::cos(povRotation);
    povForward.y = std::sin(POVPitch);
    povForward.z = std::cos(POVPitch) * std::sin(povRotation);

    float povRatio = static_cast<float>(renderTargetWidth) / renderTargetHeight;

    glm::vec3 povEye = povPosition - povForward * POVDistance;
    glm::vec3 povCenter = povEye + povForward;
    glm::mat4 povViewMatrix = glm::lookAt(povEye, povCenter, POVUp);
    glm::mat4 povProjMatrix = glm::perspective(POVFOV, povRatio, POVNear, POVFar);

    povViewProjMatrix = povProjMatrix * povViewMatrix;
}

void RendererDraw(RendererModel model, const glm::vec3& position, float rotation)
{
    modelInstances[model].Emplace(position, rotation);
}

static void PushDebugGroup(SDL_GPUCommandBuffer* commandBuffer, const std::string_view& name)
{
#ifndef NDEBUG
    SDL_PushGPUDebugGroup(commandBuffer, name.data());
#endif
}

static void InsertDebugLabel(SDL_GPUCommandBuffer* commandBuffer, const std::string_view& name)
{
#ifndef NDEBUG
    SDL_InsertGPUDebugLabel(commandBuffer, name.data());
#endif
}

static void PopDebugGroup(SDL_GPUCommandBuffer* commandBuffer)
{
#ifndef NDEBUG
    SDL_PopGPUDebugGroup(commandBuffer);
#endif
}

static void UploadModelInstances(SDL_GPUCommandBuffer* commandBuffer)
{
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
    if (!copyPass)
    {
        SDL_Log("Failed to begin copy pass: %s", SDL_GetError());
        return;
    }

    for (int i = 0; i < RendererModelCount; i++)
    {
        modelInstances[i].Upload(copyPass);
    }

    SDL_EndGPUCopyPass(copyPass);
}

static void DrawPOVModelInstances(SDL_GPUCommandBuffer* commandBuffer)
{
    SDL_GPUColorTargetInfo colorTargets[2]{};
    colorTargets[0].texture = renderTargets[RenderTargetPositionPOV];
    colorTargets[0].load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargets[0].store_op = SDL_GPU_STOREOP_STORE;
    colorTargets[0].cycle = true;
    colorTargets[1].texture = renderTargets[RenderTargetColorPOV];
    colorTargets[1].load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargets[1].store_op = SDL_GPU_STOREOP_STORE;
    colorTargets[1].cycle = true;
    SDL_GPUDepthStencilTargetInfo depthInfo{};
    depthInfo.texture = renderTargets[RenderTargetDepthPOV];
    depthInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    depthInfo.stencil_load_op = SDL_GPU_LOADOP_CLEAR;
    depthInfo.store_op = SDL_GPU_STOREOP_STORE;
    depthInfo.clear_depth = 1.0f;
    depthInfo.cycle = true;

    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, colorTargets, 2, &depthInfo);
    if (!renderPass)
    {
        SDL_Log("Failed to begin render pass: %s", SDL_GetError());
        return;
    }

    SDL_BindGPUGraphicsPipeline(renderPass, graphicsPipelines[GraphicsPipelineModelPOV]);
    SDL_PushGPUVertexUniformData(commandBuffer, 0, &povViewProjMatrix, 64);

    for (int i = 0; i < RendererModelCount; i++)
    {
        Model& model = models[i];
        Buffer<Transform>& modelInstance = modelInstances[i];
        if (!modelInstance.size || !modelInstance.buffer)
        {
            continue;
        }

        SDL_GPUBufferBinding vertexBuffers[2]{};
        SDL_GPUBufferBinding indexBuffer{};
        SDL_GPUTextureSamplerBinding textureSampler{};
        vertexBuffers[0].buffer = model.vertexBuffer;
        vertexBuffers[1].buffer = modelInstance.buffer;
        indexBuffer.buffer = model.indexBuffer;
        textureSampler.texture = model.paletteTexture;
        textureSampler.sampler = samplers[SamplerNearestClamp];
        InsertDebugLabel(commandBuffer, Models[i]);
        SDL_BindGPUVertexBuffers(renderPass, 0, vertexBuffers, 2);
        SDL_BindGPUIndexBuffer(renderPass, &indexBuffer, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_BindGPUFragmentSamplers(renderPass, 0, &textureSampler, 1);
        SDL_DrawGPUIndexedPrimitives(renderPass, model.indexCount, modelInstance.size, 0, 0, 0);
    }

    SDL_EndGPURenderPass(renderPass);
}

static void BlitToSwapchain(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUTexture* swapchainTexture)
{
    SDL_GPUBlitInfo info{};
    info.load_op = SDL_GPU_LOADOP_DONT_CARE;
    info.source.texture = renderTargets[RenderTargetColorPOV];
    info.source.w = renderTargetWidth;
    info.source.h = renderTargetHeight;
    info.destination.texture = swapchainTexture;
    info.destination.w = swapchainWidth;
    info.destination.h = swapchainHeight;
    info.filter = SDL_GPU_FILTER_NEAREST;
    SDL_BlitGPUTexture(commandBuffer, &info);
}

void RendererSubmit()
{
    SDL_WaitForGPUSwapchain(device, window);

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (!commandBuffer)
    {
        SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
        return;
    }

    SDL_GPUTexture* swapchainTexture;
    uint32_t width;
    uint32_t height;
    if (!SDL_AcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, &width, &height))
    {
        SDL_Log("Failed to acquire swapchain texture: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(commandBuffer);
        return;
    }

    if (!swapchainTexture || !width || !height)
    {
        /* can happen on e.g. window minimize */
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        return;
    }

    if (!ResizeRenderTargets(width, height))
    {
        SDL_Log("Failed to resize render targets");
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        return;
    }

    PushDebugGroup(commandBuffer, "upload_model_instances");
    UploadModelInstances(commandBuffer);
    PopDebugGroup(commandBuffer);

    PushDebugGroup(commandBuffer, "draw_pov_model_instances");
    DrawPOVModelInstances(commandBuffer);
    PopDebugGroup(commandBuffer);

    PushDebugGroup(commandBuffer, "blit_to_swapchain");
    BlitToSwapchain(commandBuffer, swapchainTexture);
    PopDebugGroup(commandBuffer);

    SDL_SubmitGPUCommandBuffer(commandBuffer);
}