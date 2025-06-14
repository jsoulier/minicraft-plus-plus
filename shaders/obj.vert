#version 450

#include "obj.glsl"

layout(location = 0) in uint inPacked;
layout(location = 1) in float inTexCoord;
layout(location = 2) in vec3 inPosition;
layout(location = 3) in float inRotation;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out flat vec2 outTexCoord;
layout(location = 2) out flat vec3 outNormal;

layout(set = 1, binding = 0) uniform uniformViewProjMatrix
{
    mat4 viewProjMatrix;
};

void main()
{
    float cosRotation = cos(inRotation);
    float sinRotation = sin(inRotation);

    mat3 rotation = mat3(
        vec3( cosRotation, 0.0f, sinRotation),
        vec3( 0.0f,        1.0f, 0.0f),
        vec3(-sinRotation, 0.0f, cosRotation)
    );

    outPosition = inPosition + rotation * objGetPosition(inPacked);
    outTexCoord = vec2(inTexCoord, 0.5f);
    outNormal = normalize(rotation * objGetNormal(inPacked));

    gl_Position = viewProjMatrix * vec4(outPosition, 1.0f);
}