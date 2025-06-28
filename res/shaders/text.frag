#version 450

layout(location = 0) in vec2 inTexcoord;
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D atlasTexture;
layout(set = 3, binding = 0) uniform uniformColor
{
    vec4 color;
};

void main()
{
    outColor = color * texture(atlasTexture, inTexcoord);
}