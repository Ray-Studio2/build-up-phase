#pragma once

#include <vector>

struct GLFWwindow;

namespace A3
{
class VulkanRendererBackend;

class Addon_imgui
{
public:
	Addon_imgui( GLFWwindow* window, VulkanRendererBackend* vulkan, std::vector<const char*>& instance_extensions );

	void renderFrame( GLFWwindow* window );
};
}