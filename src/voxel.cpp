#include <cmath>
#include <cstdint>
#include <limits>

#include "util.hpp"
#include "voxel.hpp"

MppVoxel::MppVoxel(int positionX, int positionY, int positionZ, int normalX, int normalY, int normalZ, float texCoord)
    : texCoord{texCoord}
{
    MPP_ASSERT_RELEASE(positionX > -128 && positionX < 128);
    MPP_ASSERT_RELEASE(positionY > -128 && positionY < 128);
    MPP_ASSERT_RELEASE(positionZ > -128 && positionZ < 128);

    packed = 0;
    packed |= (abs(positionX) & 0x7F) << 0;
    packed |= (positionX < 0) << 7;
    packed |= (abs(positionY) & 0x7F) << 8;
    packed |= (positionY < 0) << 15;
    packed |= (abs(positionZ) & 0x7F) << 16;
    packed |= (positionZ < 0) << 23;

    uint32_t normal = 0;
    if (normalX > 0)
    {
        normal = 0;
    }
    else if (normalX < 0)
    {
        normal = 1;
    }
    else if (normalY > 0)
    {
        normal = 2;
    }
    else if (normalY < 0)
    {
        normal = 3;
    }
    else if (normalZ > 0)
    {
        normal = 4;
    }
    else if (normalZ < 0)
    {
        normal = 5;
    }
    else
    {
        MPP_ASSERT_RELEASE(false);
    }
    packed |= (normal & 0x7) << 24;
}

bool MppVoxel::operator==(const MppVoxel other) const
{
    return packed == other.packed && abs(texCoord - other.texCoord) < std::numeric_limits<float>::epsilon();
}