#pragma once

#include <string>

namespace A3
{
struct MeshResource;

namespace Utility
{
void loadMeshFile( struct MeshResource& outMesh, const std::string& filePath );
}
}