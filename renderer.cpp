#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cstdint>
#include <unordered_map>

static constexpr const char* Title = "Minicraft Plus Plus";
static constexpr int Width = 256;
static constexpr int Height = 144;
static constexpr const char* Spritesheet = "spritesheet.png";
static constexpr const char* Font = "RasterForgeRegular.ttf";
static constexpr auto Presentation = SDL_LOGICAL_PRESENTATION_LETTERBOX;
static constexpr bool VSync = true;
static constexpr auto Flash = SDL_FLASH_BRIEFLY;
static constexpr auto PixelFormat = SDL_PIXELFORMAT_INDEX8;
static constexpr auto ScaleMode = SDL_SCALEMODE_NEAREST;
static constexpr auto BlendMode = SDL_BLENDMODE_BLEND;

static SDL_Window* window;
static SDL_Renderer* renderer;
static TTF_TextEngine* textEngine;
static SDL_Surface* spritesheet;
static SDL_Palette* palette;

static std::unordered_map<uint64_t, SDL_Surface*> spriteSurfaces;
static std::unordered_map<uint64_t, SDL_Texture*> spriteTextures;

/**
 * 00-09: color1
 * 10-19: color2
 * 20-29: color3
 * 30-39: color4
 * 40-48: x
 * 49-57: y
 * 58-61: size
 * 62-63: unused
 */
uint64_t mppRendererCreateSprite(
    uint64_t color1,
    uint64_t color2,
    uint64_t color3,
    uint64_t color4,
    uint64_t x,
    uint64_t y,
    uint64_t size)
{
    SDL_assert(color1 < 1024);
    SDL_assert(color2 < 1024);
    SDL_assert(color3 < 1024);
    SDL_assert(color4 < 1024);
    SDL_assert(x < 256);
    SDL_assert(y < 256);
    SDL_assert(size > 0 && size <= 16);

    uint64_t sprite = 0;
    sprite |= color1 << 0;
    sprite |= color2 << 10;
    sprite |= color3 << 20;
    sprite |= color4 << 30;
    sprite |= x << 40;
    sprite |= y << 49;
    sprite |= (size - 1) << 58;
    return sprite;
}

static uint64_t getSpriteSurfaceHash(uint64_t sprite)
{
    return (sprite >> 40) & 0x3FFFFF;
}

static uint64_t getSpriteTextureHash(uint64_t sprite)
{
    return sprite;
}

static SDL_Color getSpriteColor(uint64_t inColor)
{
    Uint8 red = (inColor / 100) % 10;
    Uint8 green = (inColor / 10) % 10;
    Uint8 blue = inColor % 10;

    SDL_assert(red >= 0 && red <= 7);
    SDL_assert(green >= 0 && green <= 7);
    SDL_assert(blue >= 0 && blue <= 7);

    SDL_Color color;
    color.r = red * 255 / 7;
    color.g = green * 255 / 7;
    color.b = blue * 255 / 7;
    color.a = 255;
    return color;
}

static SDL_Color getSpriteColor1(uint64_t sprite)
{
    return getSpriteColor((sprite >> 0) & 0x3FF);
}

static SDL_Color getSpriteColor2(uint64_t sprite)
{
    return getSpriteColor((sprite >> 10) & 0x3FF);
}

static SDL_Color getSpriteColor3(uint64_t sprite)
{
    return getSpriteColor((sprite >> 20) & 0x3FF);
}

static SDL_Color getSpriteColor4(uint64_t sprite)
{
    return getSpriteColor((sprite >> 30) & 0x3FF);
}

static uint64_t getSpriteX(uint64_t sprite)
{
    return (sprite >> 40) & 0x1FF;
}

static uint64_t getSpriteY(uint64_t sprite)
{
    return (sprite >> 49) & 0x1FF;
}

static uint64_t getSpriteSize(uint64_t sprite)
{
    return ((sprite >> 58) & 0xF) + 1;
}

static bool initSDL()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow(Title, 960, 720, SDL_WINDOW_HIDDEN);
    if (!window)
    {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer)
    {
        SDL_Log("Failed to create renderer: %s", SDL_GetError());
        return false;
    }

    SDL_SetRenderLogicalPresentation(renderer, Width, Height, Presentation);
    SDL_SetRenderVSync(renderer, VSync);
    SDL_SetDefaultTextureScaleMode(renderer, ScaleMode);

    SDL_ShowWindow(window);
    SDL_SetWindowResizable(window, true);
    SDL_FlashWindow(window, Flash);

    return true;
}

static bool initTTF()
{
    if (!TTF_Init())
    {
        SDL_Log("Failed to initialize SDL ttf: %s", SDL_GetError());
        return false;
    }

    textEngine = TTF_CreateRendererTextEngine(renderer);
    if (!textEngine)
    {
        SDL_Log("Failed to create text engine: %s", SDL_GetError());
        return false;
    }

    /* TODO: fonts */

    return true;
}

static bool initSpritesheet()
{
    spritesheet = IMG_Load(Spritesheet);
    if (!spritesheet)
    {
        SDL_Log("Failed to load spritesheet: %s", SDL_GetError());
        return false;
    }

    palette = SDL_CreatePalette(5);
    if (!palette)
    {
        SDL_Log("Failed to create palette: %s", SDL_GetError());
        return false;
    }

    return true;
}

bool mppRendererInit()
{
    if (!initSDL())
    {
        SDL_Log("Failed to initialize SDL");
        return false;
    }

    if (!initTTF())
    {
        SDL_Log("Failed to initialize TTF");
        return false;
    }

    if (!initSpritesheet())
    {
        SDL_Log("Failed to initialize spritesheet");
        return false;
    }

    return true;
}

static void destroySprites()
{
    for (auto& [sprite, surface] : spriteSurfaces)
    {
        SDL_DestroySurface(surface);
    }

    for (auto& [sprite, texture] : spriteTextures)
    {
        SDL_DestroyTexture(texture);
    }

    spriteSurfaces.clear();
    spriteTextures.clear();

    SDL_DestroySurface(spritesheet);
    SDL_DestroyPalette(palette);
}

void mppRendererQuit()
{
    SDL_HideWindow(window);

    destroySprites();

    TTF_DestroyRendererTextEngine(textEngine);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    TTF_Quit();
    SDL_Quit();
}

void mppRendererClear()
{
    SDL_RenderClear(renderer);
}

void mppRendererPresent()
{
    SDL_RenderPresent(renderer);
}

static SDL_Surface* createSpriteSurface(uint64_t sprite)
{
    uint64_t size = getSpriteSize(sprite);

    SDL_Rect rect;
    rect.x = getSpriteX(sprite);
    rect.y = getSpriteY(sprite);
    rect.w = size;
    rect.h = size;

    SDL_Surface* surface = SDL_CreateSurface(size, size, PixelFormat);
    if (!surface)
    {
        SDL_Log("Failed to create surface: %s", SDL_GetError());
        return nullptr;
    }

    SDL_BlitSurface(spritesheet, &rect, surface, nullptr);

    return surface;
}

static SDL_Texture* createSpriteTexture(uint64_t sprite, SDL_Surface* surface)
{
    SDL_Color colors[5];
    colors[0] = getSpriteColor1(sprite);
    colors[1] = getSpriteColor2(sprite);
    colors[2] = getSpriteColor3(sprite);
    colors[3] = getSpriteColor4(sprite);
    colors[4].a = 0.0f;

    SDL_SetPaletteColors(palette, colors, 0, 5);
    SDL_SetSurfacePalette(surface, palette);

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture)
    {
        SDL_Log("Failed to create texture: %s", SDL_GetError());
        return nullptr;
    }

    SDL_SetTextureBlendMode(texture, BlendMode);

    return texture;
}

void mppRendererDraw(uint64_t sprite, float x, float y)
{
    uint64_t surfaceHash = getSpriteSurfaceHash(sprite);
    uint64_t textureHash = getSpriteTextureHash(sprite);

    auto surface = spriteSurfaces.find(surfaceHash);
    if (surface == spriteSurfaces.end())
    {
        SDL_Surface* spriteSurface = createSpriteSurface(sprite);
        surface = spriteSurfaces.emplace(surfaceHash, spriteSurface).first;
    }
    if (!surface->second)
    {
        return;
    }

    auto texture = spriteTextures.find(textureHash);
    if (texture == spriteTextures.end())
    {
        SDL_Texture* spriteTexture = createSpriteTexture(sprite, surface->second);
        texture = spriteTextures.emplace(textureHash, spriteTexture).first;
    }
    if (!texture->second)
    {
        return;
    }

    uint64_t size = getSpriteSize(sprite);

    SDL_FRect rect;
    rect.x = x;
    rect.y = y;
    rect.w = size;
    rect.h = size;

    SDL_RenderTexture(renderer, texture->second, nullptr, &rect);
}