//
// Created by NeftyOliver on 2025-03-24.
//

#include "vulkan_utility.h"

#include "Test.h"


bool testIsLayerSupported() {
    return vkut::isLayerSupported("VK_LAYER_KHRONOS_validation");
}

bool testInstanceLayer() {
    std::vector<std::string> instanceExtensionNames = {
        // 3 layers and 2 dummy
        "VK_LAYER_KHRONOS_validation",
        "Not a layer~",
        "VK_LAYER_KHRONOS_synchronization2",
        "VK_LAYER_LUNARG_screenshot",
        "dummy2"
    };


    vkut::filterSupportedInstanceLayers(instanceExtensionNames);


    for (auto str: instanceExtensionNames) {
        std::cout << "    Selected instance layers can be used: " << str << std::endl;
    }

    //std::cout << vecFiltered.capacity() << std::endl;

    return instanceExtensionNames.capacity() == 3;

}

int main() {
    printTest("InstanceLayer Support", testIsLayerSupported());
    printTest("InstanceLayer Filter", testInstanceLayer());

    std::cout << "VulkanUtility test done" << std::endl;
}

