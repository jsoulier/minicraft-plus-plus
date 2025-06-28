#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in flat vec2 inTexcoord;
layout(location = 2) in flat vec3 inNormal;
layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D paletteTexture;

void main()
{
    outPosition = vec4(inPosition, 0.0f);
    outColor = texture(paletteTexture, inTexcoord);
}