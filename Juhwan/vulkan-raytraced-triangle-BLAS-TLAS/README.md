## 학습내용 Overview
- 레이트레이싱 extention 사용하기
- Acceleration Structure 빌드 (BLAS & TLAS)
- 레이트레이싱 쉐이더 (rayGen, chit, miss 등등)
- 레이트레이싱 파이프라인
- 쉐이더 바인딩 테이블


## 레이트레이싱 extention 사용하기

#### 레이트레이싱 extention 활성화
```cpp
void createVkDevice()
{
    ...

    std::vector<const char*> extentions = { 
        VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    };

    ...

    VkDeviceCreateInfo createInfo{
        ...
        .enabledExtensionCount = (uint)extentions.size(),
        .ppEnabledExtensionNames = extentions.data(),
    };

    VkPhysicalDeviceAccelerationStructureFeaturesKHR f1{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        .accelerationStructure = VK_TRUE,
    };

    VkPhysicalDeviceBufferDeviceAddressFeatures f2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .bufferDeviceAddress = VK_TRUE,
    };

    createInfo.pNext = &f1;
    f1.pNext = &f2;

    if (vkCreateDevice(vk.physicalDevice, &createInfo, nullptr, &vk.device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    ...
}
```

#### 다이나믹 함수 로드
```cpp
struct Global {
    PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    ...
};

void loadDeviceExtensionFunctions(VkDevice device)
{
    vk.vkGetBufferDeviceAddressKHR = (PFN_vkGetBufferDeviceAddressKHR)(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
    vk.vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
    vk.vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
    vk.vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
    vk.vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
    vk.vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
}

int main()
{
    ...
    createVkDevice();
    loadDeviceExtensionFunctions(vk.device);
    ...
}
```

#### VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT usage 플래그
```cpp
std::tuple<VkBuffer, VkDeviceMemory> createBuffer(
    VkDeviceSize size, 
    VkBufferUsageFlags usage, 
    VkMemoryPropertyFlags reqMemProps)
{
    ...

    VkMemoryAllocateInfo allocInfo{
        ...
    };

    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        VkMemoryAllocateFlagsInfo flagsInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
            .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR,
        };
        allocInfo.pNext = &flagsInfo;
    }

    if (vkAllocateMemory(vk.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    ...
}
```

## 레퍼런런스
[이론]
- https://www.khronos.org/blog/ray-tracing-in-vulkan
- https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/vkrt_tutorial.md.html
- https://nvpro-samples.github.io/vk_mini_path_tracer/index.html
- https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html

[코드]
- https://github.com/KhronosGroup/Vulkan-Samples/tree/main/samples/extensions
- https://github.com/SaschaWillems/Vulkan
- https://github.com/nvpro-samples/vk_raytracing_tutorial_KHR
- https://github.com/nvpro-samples/vk_mini_path_tracer


## 과제
1. obj파일 로드 -> Acceleration structure 빌드 성공하기