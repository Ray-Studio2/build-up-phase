
#include <iostream>
#include <ostream>
#include <vector>
#include <vulkan/vulkan.h>


constexpr char * const* enabledExtensionNames = {
};
constexpr uint32_t enabledExtensionCount = sizeof(enabledExtensionNames) / sizeof(char*);

constexpr char * const enabledLayerNames[] = {
    "VK_LAYER_KHRONOS_validation"
};
constexpr uint32_t enabledLayerCount = sizeof(enabledLayerNames) / sizeof(char*);



int main () {

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    for (uint32_t i = 0; i < extensionCount; i++) {
        std::string extensionName = extensions[i].extensionName;

        std::cout << extensionName << std::endl;
    }

    std::cout << std::endl << std::endl;

    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
    for (uint32_t i = 0; i < layerCount; i++) {
        std::string layerName = layers[i].layerName;
        std::cout << layerName << std::endl;
    }

    VkInstance instance;

    auto applicationInfo = VkApplicationInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "Utility",
        .applicationVersion = 0,
        .pEngineName = "",
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_3,
    };

    auto instanceCreateInfo = VkInstanceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = enabledLayerCount,
        .ppEnabledLayerNames = enabledLayerNames,
        .enabledExtensionCount = enabledExtensionCount,
        .ppEnabledExtensionNames = enabledExtensionNames
    };

    vkCreateInstance(&instanceCreateInfo, VK_NULL_HANDLE, &instance);




    /*
    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
    auto vecPhysicalDevices = std::vector<VkPhysicalDevice>(physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, vecPhysicalDevices.data());

    std::cout << "physical device count: " << physicalDeviceCount << std::endl;
    */

    vkDestroyInstance(instance, nullptr);

    return 0;
}