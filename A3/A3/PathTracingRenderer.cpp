#include "PathTracingRenderer.h"
#include "RenderSettings.h"
#include "Vulkan.h"

using namespace A3;

PathTracingRenderer::PathTracingRenderer( VulkanRendererBackend* inBackend )
	: backend( inBackend )
{
    backend->createBLAS();
    backend->createTLAS();
    backend->createOutImage();
    backend->createUniformBuffer();
    backend->createRayTracingPipeline();
    backend->createDescriptorSets();
    backend->createShaderBindingTable();
}

void PathTracingRenderer::beginFrame()
{
    backend->beginFrame();
}

void PathTracingRenderer::render()
{
	backend->beginRaytracingPipeline();
}

void PathTracingRenderer::endFrame()
{
    backend->endFrame();
}
