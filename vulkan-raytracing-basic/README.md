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
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, // not used
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, // not used
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,

        VK_KHR_SPIRV_1_4_EXTENSION_NAME, // not used
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    };

    ...

    VkDeviceCreateInfo createInfo{
        ...
        .enabledExtensionCount = (uint)extentions.size(),
        .ppEnabledExtensionNames = extentions.data(),
    };

    VkPhysicalDeviceBufferDeviceAddressFeatures f1{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .bufferDeviceAddress = VK_TRUE,
    };

	VkPhysicalDeviceAccelerationStructureFeaturesKHR f2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        .accelerationStructure = VK_TRUE,
    };
	
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR f3{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
        .rayTracingPipeline = VK_TRUE,
    }; 

    createInfo.pNext = &f1;
    f1.pNext = &f2;
    f2.pNext = &f3;

    if (vkCreateDevice(vk.physicalDevice, &createInfo, nullptr, &vk.device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    ...
}
```

#### 다이나믹 함수 로드
```cpp
void loadDeviceExtensionFunctions(VkDevice device)
{
    vk.vkGetBufferDeviceAddressKHR = (PFN_vkGetBufferDeviceAddressKHR)(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
    vk.vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
    vk.vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
    vk.vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
    vk.vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
    vk.vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
    vk.vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
	vk.vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)(vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
    vk.vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));

    VkPhysicalDeviceProperties2 deviceProperties2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &vk.rtProperties,
    };
	vkGetPhysicalDeviceProperties2(vk.physicalDevice, &deviceProperties2);

    if (vk.rtProperties.shaderGroupHandleSize != SHADER_GROUP_HANDLE_SIZE) {
        throw std::runtime_error("shaderGroupHandleSize must be 32 mentioned in the vulakn spec (Table 69. Required Limits)!");
    }
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

#### Swap Chain
- Remove all "vk.swapChainImageViews" related references.
```cpp
void createSwapChain()
{
    ...
    VkSwapchainCreateInfoKHR createInfo{
        ...
        .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT, // VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        ...
    };
    ...

    // VkImageView imageView;
    // if (vkCreateImageView(vk.device, &createInfo, nullptr, &imageView) != VK_SUCCESS) {
    //     throw std::runtime_error("failed to create image views!");
    // }
    // vk.swapChainImageViews.push_back(imageView);
    ...
}
```

#### 전역변수
```cpp
struct Global {
    PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
	PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR  rtProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
    
    ...
    // std::vector<VkImageView> swapChainImageViews;
    ...

    VkBuffer blasBuffer;
    VkDeviceMemory blasBufferMem;
    VkAccelerationStructureKHR blas;
    VkDeviceAddress blasAddress;

    VkBuffer tlasBuffer;
    VkDeviceMemory tlasBufferMem;
    VkAccelerationStructureKHR tlas;

    VkImage outImage;
    VkDeviceMemory outImageMem;
    VkImageView outImageView;

    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMem;

    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;

    VkBuffer sbtBuffer;
    VkDeviceMemory sbtBufferMem;
    VkStridedDeviceAddressRegionKHR rgenSbt{};
    VkStridedDeviceAddressRegionKHR missSbt{};
    VkStridedDeviceAddressRegionKHR hitgSbt{};

    ~Global() {
        ...
};
```


## 레퍼런런스
[Spec]
- https://github.com/KhronosGroup/GLSL/blob/main/extensions/ext/GLSL_EXT_ray_tracing.txt
- https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html

[이론]
- https://www.khronos.org/blog/ray-tracing-in-vulkan
- https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/vkrt_tutorial.md.html
- https://nvpro-samples.github.io/vk_mini_path_tracer/index.html

[코드]
- https://github.com/KhronosGroup/Vulkan-Samples/tree/main/samples/extensions
- https://github.com/SaschaWillems/Vulkan
- https://github.com/nvpro-samples/vk_raytracing_tutorial_KHR
- https://github.com/nvpro-samples/vk_mini_path_tracer



## 퀴즈:
[Quiz1] 레이트레이싱 아웃 대상 : 스토리지 이미지 -> 스토리지 버퍼 변경  
[Quiz2] render()부분 동기화 수정 및 최적화

## 과제
1. obj파일 로드 -> Acceleration structure 빌드 성공하기
