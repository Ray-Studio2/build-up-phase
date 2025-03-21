#pragma once

namespace A3
{
class VulkanRendererBackend;

class PathTracingRenderer
{
public:
	PathTracingRenderer( VulkanRendererBackend* inBackend );

	void beginFrame();
	void render();
	void endFrame();

private:
	VulkanRendererBackend* backend;
};
}