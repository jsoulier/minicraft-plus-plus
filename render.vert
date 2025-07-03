#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in uint inInstance;
layout(set = 0, binding = 0, r8ui) uniform readonly uimage3D cells;
layout(set = 1, binding = 0) uniform uniformViewProjMatrix
{
    mat4 viewProjMatrix;
};

void main()
{
    ivec3 instance;
    instance.x = int((inInstance >>  0) & 0x3FF);
    instance.y = int((inInstance >> 10) & 0x3FF);
    instance.z = int((inInstance >> 20) & 0x3FF);
    if (imageLoad(cells, instance).x > 0)
    {
        gl_Position = viewProjMatrix * vec4(inPosition + vec3(instance), 1.0f);
    }
    else
    {
        gl_Position = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }
}