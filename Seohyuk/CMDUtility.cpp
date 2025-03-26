
#include <iostream>
#include <ostream>
#include <vector>
#include <vulkan/vulkan.h>


std::vector<const char *> enabledExtensionNames = {
    "VK_KHR_get_physical_device_properties2"
};
const uint32_t enabledExtensionCount = enabledExtensionNames.capacity();

std::vector<const char *> enabledLayerNames = {
    "VK_LAYER_KHRONOS_validation"
};
const uint32_t enabledLayerCount = enabledLayerNames.capacity();



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

    const auto applicationInfo = VkApplicationInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "Utility",
        .applicationVersion = 0,
        .pEngineName = "None",
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_4,
    };

    auto instanceCreateInfo = VkInstanceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = enabledLayerCount,
        .ppEnabledLayerNames = enabledLayerNames.data(),
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = enabledExtensionNames.data()
    };

    vkCreateInstance(&instanceCreateInfo, VK_NULL_HANDLE, &instance);





    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
    auto vecPhysicalDevices = std::vector<VkPhysicalDevice>(physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, vecPhysicalDevices.data());

    std::cout << "physical device count: " << physicalDeviceCount << std::endl;


    vkDestroyInstance(instance, nullptr);

    return 0;
}