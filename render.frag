#version 450

#include "config.hpp"

layout(location = 0) in flat uint inValue;
layout(location = 0) out vec4 outColor;
layout(set = 3, binding = 0) uniform uniformRules
{
    uint seed;
    uint surviveMask;
    uint birthMask;
    uint life;
    uint neighborhood;
    uint frame;
};

void main()
{
    vec3 color1 = vec3(1.0f, 1.0f, 0.0f);
    vec3 color2 = vec3(1.0f, 0.0f, 1.0f);
    outColor = vec4(mix(color1, color2, float(inValue) / float(life)), 1.0f);
}