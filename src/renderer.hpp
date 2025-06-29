#pragma once

#include <glm/glm.hpp>
#include <string_view>

enum RendererModel
{
    RendererModelGrass,
    RendererModelCount,
};

bool RendererInit();
void RendererQuit();
void RendererMove(const glm::vec3& position, float rotation);
void RendererDraw(RendererModel model, const glm::vec3& position, float rotation);
void RendererDraw(const std::string_view& text, const glm::vec2& position, const glm::vec4& color, int size);
void RendererSubmit();