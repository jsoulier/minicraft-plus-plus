#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>

#include <cmath>
#include <cstdint>
#include <cstring>

#include "config.hpp"
#include "shader.hpp"

static_assert(BOUNDS < 1024);

static SDL_Window* window;
static SDL_GPUDevice* device;
static SDL_GPUGraphicsPipeline* graphicsPipeline;
static SDL_GPUComputePipeline* computePipeline;
static SDL_GPUTexture* textures[FRAMES];
static SDL_GPUBuffer* vertexBuffer;
static SDL_GPUBuffer* instanceBuffer;

struct
{
    uint32_t survival{4};
    uint32_t birth{4};
    uint32_t state{5};
    uint32_t neighborhood{MOORE};
    uint32_t frame{0};
}
static rules;

static float pitch;
static float yaw;
static float distance;
static int readFrame{0};
static int writeFrame{1};

static bool Init()
{
    SDL_SetAppMetadata("3D Cellular Automata", nullptr, nullptr);
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return false;
    }
    window = SDL_CreateWindow("3D Cellular Automata", 960, 720, SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return false;
    }
#if defined(SDL_PLATFORM_WIN32)
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL, true, nullptr);
#elif defined(SDL_PLATFORM_APPLE)
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_MSL, true, nullptr);
#else
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr);
#endif
    if (!device)
    {
        SDL_Log("Failed to create device: %s", SDL_GetError());
        return false;
    }
    if (!SDL_ClaimWindowForGPUDevice(device, window))
    {
        SDL_Log("Failed to create swapchain: %s", SDL_GetError());
        return false;
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForSDLGPU(window);
    ImGui_ImplSDLGPU3_InitInfo info{};
    info.Device = device;
    info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(device, window);
    ImGui_ImplSDLGPU3_Init(&info);
    return true;
}

static bool CreatePipelines()
{
    SDL_GPUShader* vertShader = LoadShader(device, "render.vert");
    SDL_GPUShader* fragShader = LoadShader(device, "render.frag");
    if (!vertShader || !fragShader)
    {
        SDL_Log("Failed to load shader(s)");
        return false;
    }
    SDL_GPUVertexBufferDescription buffers[2] =
    {{
        .slot = 0,
        .pitch = sizeof(float) * 3,
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        .instance_step_rate = 0,
    },
    {
        .slot = 1,
        .pitch = sizeof(uint32_t),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
        .instance_step_rate = 0,
    }};
    SDL_GPUVertexAttribute attribs[2] =
    {{
        .location = 0,
        .buffer_slot = 0,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
        .offset = 0,
    },
    {
        .location = 1,
        .buffer_slot = 1,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_UINT,
        .offset = 0,
    }};
    SDL_GPUColorTargetDescription targets[1] =
    {{
        .format = SDL_GetGPUSwapchainTextureFormat(device, window),
    }};
    SDL_GPUGraphicsPipelineCreateInfo info{};
    info.vertex_shader = vertShader;
    info.fragment_shader = fragShader;
    info.vertex_input_state.vertex_buffer_descriptions = buffers;
    info.vertex_input_state.num_vertex_buffers = 2;
    info.vertex_input_state.vertex_attributes = attribs;
    info.vertex_input_state.num_vertex_attributes = 2;
    info.target_info.color_target_descriptions = targets;
    info.target_info.num_color_targets = 1;
    graphicsPipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
    computePipeline = LoadComputePipeline(device, "automata.comp");
    if (!graphicsPipeline || !computePipeline)
    {
        SDL_Log("Failed to create pipeline(s): %s", SDL_GetError());
        return false;
    }
    SDL_ReleaseGPUShader(device, vertShader);
    SDL_ReleaseGPUShader(device, fragShader);
    return true;
}

static bool CreateResources()
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
    for (int i = 0; i < FRAMES; i++)
    {
        SDL_GPUTextureCreateInfo info{};
        info.type = SDL_GPU_TEXTURETYPE_3D;
        info.format = SDL_GPU_TEXTUREFORMAT_R8_UINT;
        info.usage =
            SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ |
            SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE |
            SDL_GPU_TEXTUREUSAGE_GRAPHICS_STORAGE_READ;
        info.width = BOUNDS;
        info.height = BOUNDS;
        info.layer_count_or_depth = BOUNDS;
        info.num_levels = 1;
        textures[i] = SDL_CreateGPUTexture(device, &info);
        if (!textures[i])
        {
            SDL_Log("Failed to create texture: %s", SDL_GetError());
            return false;
        }
    }
    {
        float vertices[36 * 3] =
        {
           -0.5f,-0.5f, 0.5f,
            0.5f,-0.5f, 0.5f,
            0.5f, 0.5f, 0.5f,
           -0.5f,-0.5f, 0.5f,
            0.5f, 0.5f, 0.5f,
           -0.5f, 0.5f, 0.5f,
           -0.5f,-0.5f,-0.5f,
            0.5f, 0.5f,-0.5f,
            0.5f,-0.5f,-0.5f,
           -0.5f,-0.5f,-0.5f,
           -0.5f, 0.5f,-0.5f,
            0.5f, 0.5f,-0.5f,
           -0.5f,-0.5f,-0.5f,
           -0.5f,-0.5f, 0.5f,
           -0.5f, 0.5f, 0.5f,
           -0.5f,-0.5f,-0.5f,
           -0.5f, 0.5f, 0.5f,
           -0.5f, 0.5f,-0.5f,
            0.5f,-0.5f,-0.5f,
            0.5f, 0.5f, 0.5f,
            0.5f,-0.5f, 0.5f,
            0.5f,-0.5f,-0.5f,
            0.5f, 0.5f,-0.5f,
            0.5f, 0.5f, 0.5f,
           -0.5f, 0.5f,-0.5f,
           -0.5f, 0.5f, 0.5f,
            0.5f, 0.5f, 0.5f,
           -0.5f, 0.5f,-0.5f,
            0.5f, 0.5f, 0.5f,
            0.5f, 0.5f,-0.5f,
           -0.5f,-0.5f,-0.5f,
            0.5f,-0.5f, 0.5f,
           -0.5f,-0.5f, 0.5f,
           -0.5f,-0.5f,-0.5f,
            0.5f,-0.5f,-0.5f,
            0.5f,-0.5f, 0.5f,
        };
        SDL_GPUTransferBuffer* transferBuffer;
        {
            SDL_GPUTransferBufferCreateInfo info{};
            info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            info.size = sizeof(vertices);
            transferBuffer = SDL_CreateGPUTransferBuffer(device, &info);
            if (!transferBuffer)
            {
                SDL_Log("Failed to create transfer buffer: %s", SDL_GetError());
                return false;
            }
        }
        void* data = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
        if (!data)
        {
            SDL_Log("Failed to map transfer buffer: %s", SDL_GetError());
            return false;
        }
        {
            SDL_GPUBufferCreateInfo info{};
            info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
            info.size = sizeof(vertices);
            vertexBuffer = SDL_CreateGPUBuffer(device, &info);
            if (!vertexBuffer)
            {
                SDL_Log("Failed to create buffer: %s", SDL_GetError());
                return false;
            }
        }
        std::memcpy(data, vertices, sizeof(vertices));
        SDL_GPUTransferBufferLocation location{};
        SDL_GPUBufferRegion region{};
        location.transfer_buffer = transferBuffer;
        region.buffer = vertexBuffer;
        region.size = sizeof(vertices);
        SDL_UploadToGPUBuffer(copyPass, &location, &region, false);
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
    }
    {
        SDL_GPUTransferBuffer* transferBuffer;
        {
            SDL_GPUTransferBufferCreateInfo info{};
            info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            info.size = BOUNDS * BOUNDS * BOUNDS * sizeof(uint32_t);
            transferBuffer = SDL_CreateGPUTransferBuffer(device, &info);
            if (!transferBuffer)
            {
                SDL_Log("Failed to create transfer buffer: %s", SDL_GetError());
                return 1;
            }
        }
        uint32_t* data = static_cast<uint32_t*>(SDL_MapGPUTransferBuffer(device, transferBuffer, false));
        if (!data)
        {
            SDL_Log("Failed to map transfer buffer: %s", SDL_GetError());
            return false;
        }
        {
            SDL_GPUBufferCreateInfo info{};
            info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
            info.size = BOUNDS * BOUNDS * BOUNDS * sizeof(uint32_t);
            instanceBuffer = SDL_CreateGPUBuffer(device, &info);
            if (!instanceBuffer)
            {
                SDL_Log("Failed to create buffer: %s", SDL_GetError());
                return false;
            }
        }
        uint32_t i = 0;
        for (int x = 0; x < BOUNDS; x++)
        for (int y = 0; y < BOUNDS; y++)
        for (int z = 0; z < BOUNDS; z++)
        {
            data[i] = 0;
            data[i] |= x << 0;
            data[i] |= y << 10;
            data[i] |= z << 20;
            i++;
        }
        SDL_GPUTransferBufferLocation location{};
        SDL_GPUBufferRegion region{};
        location.transfer_buffer = transferBuffer;
        region.buffer = instanceBuffer;
        region.size = BOUNDS * BOUNDS * BOUNDS * sizeof(uint32_t);
        SDL_UploadToGPUBuffer(copyPass, &location, &region, false);
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
    }
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
    return true;
}

static void Draw()
{
    SDL_WaitForGPUSwapchain(device, window);
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (!commandBuffer)
    {
        SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
        return;
    }
    SDL_GPUTexture* texture;
    uint32_t width;
    uint32_t height;
    if (!SDL_AcquireGPUSwapchainTexture(commandBuffer, window, &texture, &width, &height))
    {
        SDL_Log("Failed to acquire swapchain texture: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(commandBuffer);
        return;
    }
    if (!texture || !width || !height)
    {
        /* happens on minimize */
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        return;
    }
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = width;
    io.DisplaySize.y = height;
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui::NewFrame();
    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();
    ImGui_ImplSDLGPU3_PrepareDrawData(drawData, commandBuffer);
    {
        SDL_GPUStorageTextureReadWriteBinding textureBinding{};
        textureBinding.texture = textures[writeFrame];
        SDL_GPUComputePass* computePass = SDL_BeginGPUComputePass(commandBuffer, &textureBinding, 1, nullptr, 0);
        if (!computePass)
        {
            SDL_Log("Failed to begin compute pass: %s", SDL_GetError());
            SDL_SubmitGPUCommandBuffer(commandBuffer);
            return;
        }
        SDL_BindGPUComputePipeline(computePass, computePipeline);
        SDL_BindGPUComputeStorageTextures(computePass, 0, &textures[readFrame], 1);
        int groups = (BOUNDS + THREADS - 1) / THREADS;
        SDL_DispatchGPUCompute(computePass, groups, groups, groups);
        SDL_EndGPUComputePass(computePass);
    }
    {
        SDL_GPUColorTargetInfo info{};
        info.texture = texture;
        info.load_op = SDL_GPU_LOADOP_CLEAR;
        info.store_op = SDL_GPU_STOREOP_STORE;
        SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &info, 1, nullptr);
        if (!renderPass)
        {
            SDL_Log("Failed to begin render pass: %s", SDL_GetError());
            SDL_SubmitGPUCommandBuffer(commandBuffer);
            return;
        }
        ImGui_ImplSDLGPU3_RenderDrawData(drawData, commandBuffer, renderPass);
        SDL_EndGPURenderPass(renderPass);
    }
    SDL_SubmitGPUCommandBuffer(commandBuffer);
}

int main(int argc, char** argv)
{
    if (!Init())
    {
        SDL_Log("Failed to initialize");
        return 1;
    }
    if (!CreatePipelines())
    {
        SDL_Log("Failed to create pipelines");
        return 1;
    }
    if (!CreateResources())
    {
        SDL_Log("Failed to create resources");
        return 1;
    }
    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            }
        }
        if (!running)
        {
            break;
        }
        Draw();
        readFrame = (readFrame + 1) % FRAMES;
        writeFrame = (writeFrame + 1) % FRAMES;
        rules.frame++;
    }
    for (int i = 0; i < FRAMES; i++)
    {
        SDL_ReleaseGPUTexture(device, textures[i]);
    }
    SDL_ReleaseGPUBuffer(device, vertexBuffer);
    SDL_ReleaseGPUBuffer(device, instanceBuffer);
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_ReleaseGPUGraphicsPipeline(device, graphicsPipeline);
    SDL_ReleaseGPUComputePipeline(device, computePipeline);
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}