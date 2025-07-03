#version 450

#include "config.hpp"

layout(location = 0) in flat ivec3 inInstance;
layout(location = 0) out vec4 outColor;

void main()
{
    float distance1 = length(vec3(BOUNDS)) / 2.0f;
    float distance2 = distance(vec3(inInstance), vec3(BOUNDS) / 2.0f);
    vec3 color1 = vec3(1.0f, 1.0f, 0.0f);
    vec3 color2 = vec3(1.0f, 0.0f, 1.0f);
    outColor = vec4(mix(color1, color2, distance2 / distance1), 1.0f);
}