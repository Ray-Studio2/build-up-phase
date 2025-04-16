
/***
 *
 * WTFPL Public license
 *
 * 0. You just DO WHAT THE **** YOU WANT TO.
 *
 */

/**
 *
 */

#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>

namespace vkut {
 VkBool32 checkValidationLayerSupport() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, NULL);
  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
  for (uint32_t i = 0; i < layerCount; i++) {
   if (availableLayers[i].layerName == "VK_LAYER_KHRONOS_validation") {
    std::cerr << "Validation layer supports!" << availableLayers.at(i).layerName << std::endl;
    return VK_TRUE;
   }
  }
  return VK_FALSE;
 }

void showInstanceLayers() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, NULL);
  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (uint32_t i = 0; i < layerCount; i++) {
   std::cout << availableLayers.at(i).layerName << std::endl;
  }
 }

 void showInstanceExtensions() {
  uint32_t propertyCount;
  std::vector<VkExtensionProperties> pProperties;

  vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, nullptr);
  pProperties.resize(propertyCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, pProperties.data());

  for (uint32_t i = 0; i < propertyCount; i += 1) {
   std::cout << pProperties.at(i).extensionName << std::endl;
  }
 }
}