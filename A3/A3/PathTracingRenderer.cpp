#include "PathTracingRenderer.h"
#include "RenderSettings.h"
#include "Vulkan.h"
#include "Scene.h"
#include "MeshObject.h"
#include "MeshResource.h"

using namespace A3;

PathTracingRenderer::PathTracingRenderer( VulkanRendererBackend* inBackend )
	: backend( inBackend )
{
}

void PathTracingRenderer::beginFrame( int32 screenWidth, int32 screenHeight ) const
{
    backend->beginFrame( screenWidth, screenHeight );
}

void PathTracingRenderer::render( const Scene& scene ) const
{
    if( scene.isSceneDirty() )
    {
        std::vector<MeshObject*> meshObjects = scene.collectMeshObjects();
        buildBlas( meshObjects[ 0 ] );

        backend->rebuildAccelerationStructure();
    }

    backend->beginRaytracingPipeline();
}

void PathTracingRenderer::endFrame() const
{
    backend->endFrame();
}

void PathTracingRenderer::buildBlas( MeshObject* meshObject ) const
{
    MeshResource* resource = meshObject->getResource();
    
    VkAccelerationStructureKHR blas = backend->createBLAS( resource->vertexData, resource->indexData );
}