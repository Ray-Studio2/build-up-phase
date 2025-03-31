#pragma once

#include <vector>
#include "EngineTypes.h"

namespace A3
{
struct Vertex
{
    float position[ 4 ];
    float normal[ 4 ];
    float color[ 4 ];
    float uv[ 4 ];
};

struct MeshResource
{
    std::vector<Vertex> vertexData;
    std::vector<uint32> indexData;
};
}