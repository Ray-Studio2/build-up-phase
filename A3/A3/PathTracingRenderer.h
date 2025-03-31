#pragma once

#include "EngineTypes.h"

namespace A3
{
class VulkanRendererBackend;
class Scene;
class MeshObject;

class PathTracingRenderer
{
public:
	PathTracingRenderer( VulkanRendererBackend* inBackend );

	void beginFrame( int32 screenWidth, int32 screenHeight ) const;
	void render( const Scene& scene ) const;
	void endFrame() const;

	void buildBlas( MeshObject* meshObject ) const;

private:
	VulkanRendererBackend* backend;
};
}