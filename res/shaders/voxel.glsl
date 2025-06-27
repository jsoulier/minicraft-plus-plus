#ifndef VOXEL_GLSL
#define VOXEL_GLSL

vec3 VoxelGetPosition(uint packed)
{
    ivec3 direction;
    direction.x = 1 - 2 * int((packed >> 7) & 0x1);
    direction.y = 1 - 2 * int((packed >> 15) & 0x1);
    direction.z = 1 - 2 * int((packed >> 23) & 0x1);

    ivec3 magnitude;
    magnitude.x = int((packed >> 0) & 0x7F);
    magnitude.y = int((packed >> 8) & 0x7F);
    magnitude.z = int((packed >> 16) & 0x7F);

    return magnitude * direction;
}

vec3 VoxelGetNormal(uint packed)
{
    const vec3 Directions[6] = vec3[]
    (
        vec3( 1.0f, 0.0f, 0.0f),
        vec3(-1.0f, 0.0f, 0.0f),
        vec3( 0.0f, 1.0f, 0.0f),
        vec3( 0.0f,-1.0f, 0.0f),
        vec3( 0.0f, 0.0f, 1.0f),
        vec3( 0.0f, 0.0f,-1.0f)
    );

    return Directions[int((packed >> 24) & 0x7)];
}

#endif