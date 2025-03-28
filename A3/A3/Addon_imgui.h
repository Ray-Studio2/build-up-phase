#pragma once

#include "EngineTypes.h"
#include <vector>

struct GLFWwindow;
struct ImGui_ImplVulkanH_Window;

namespace A3
{
class VulkanRendererBackend;

class Addon_imgui
{
public:
	Addon_imgui( GLFWwindow* window, VulkanRendererBackend* vulkan, int32 screenWidth, int32 screenHeight );

	void renderFrame( GLFWwindow* window, VulkanRendererBackend* vulkan );

private:
	// @FIXME: Temporary. Remove later.
	void CleanupVulkan( VulkanRendererBackend* vulkan );
	void CleanupVulkanWindow( VulkanRendererBackend* vulkan );
};
}