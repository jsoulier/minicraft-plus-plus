#pragma once

#include <cstdint>
#include <functional>

struct MppVoxel
{
    MppVoxel(int positionX, int positionY, int positionZ, int normalX, int normalY, int normalZ, float texCoord);
    bool operator==(const MppVoxel other) const;

    uint32_t packed;
    float texCoord;
};

namespace std
{

template<>
struct hash<MppVoxel>
{
    size_t operator()(const MppVoxel voxel) const
    {
        return std::hash<uint32_t>{}(voxel.packed) ^ std::hash<float>{}(voxel.texCoord);
    }
};

}