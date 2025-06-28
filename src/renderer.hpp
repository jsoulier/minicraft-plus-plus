#pragma once

#include <glm/glm.hpp>

enum RendererModel
{
    RendererModelGrass,
    RendererModelCount,
};

bool RendererInit();
void RendererQuit();
void RendererMove(const glm::vec3& position, float rotation);
void RendererDraw(RendererModel model, const glm::vec3& position, float rotation);
void RendererSubmit();