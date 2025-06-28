#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexcoord;
layout(location = 0) out vec2 outTexcoord;
layout(set = 1, binding = 0) uniform uniformProjMatrix
{
    mat4 projMatrix;
};
layout(set = 1, binding = 1) uniform uniformPosition
{
    vec2 position;
};

void main()
{
    outTexcoord = inTexcoord;
    gl_Position = projMatrix * vec4(inPosition + position, 0.0f, 1.0f);
}