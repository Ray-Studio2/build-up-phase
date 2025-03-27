add_subdirectory( glfw )

if (${CMAKE_SYSTEM_NAME} STREQUAL Darwin) # macOS

    #adding MoltenVK in to the project.

    enable_language(OBJC)
    enable_language(OBJCXX)

    add_library(MoltenVK SHARED
            MoltenVK/Common/MVKCommonEnvironment.h
            MoltenVK/Common/MVKOSExtensions.h
            MoltenVK/Common/MVKOSExtensions.mm
            MoltenVK/Common/MVKStrings.h

            MoltenVK/MoltenVK/include/MoltenVK/mvk_config.h
            MoltenVK/MoltenVK/include/MoltenVK/mvk_datatypes.h
            MoltenVK/MoltenVK/include/MoltenVK/mvk_deprecated_api.h
            MoltenVK/MoltenVK/include/MoltenVK/mvk_private_api.h
            MoltenVK/MoltenVK/include/MoltenVK/mvk_vulkan.h
            MoltenVK/MoltenVK/include/MoltenVK/vk_mvk_moltenvk.h

            MoltenVK/MoltenVK/include/vk_video/vulkan_video_codec_av1std.h
            MoltenVK/MoltenVK/include/vk_video/vulkan_video_codec_av1std_decode.h
            MoltenVK/MoltenVK/include/vk_video/vulkan_video_codec_av1std_encode.h
            MoltenVK/MoltenVK/include/vk_video/vulkan_video_codec_h264std.h
            MoltenVK/MoltenVK/include/vk_video/vulkan_video_codec_h264std_decode.h
            MoltenVK/MoltenVK/include/vk_video/vulkan_video_codec_h264std_encode.h
            MoltenVK/MoltenVK/include/vk_video/vulkan_video_codec_h265std.h
            MoltenVK/MoltenVK/include/vk_video/vulkan_video_codec_h265std_decode.h
            MoltenVK/MoltenVK/include/vk_video/vulkan_video_codec_h265std_encode.h
            MoltenVK/MoltenVK/include/vk_video/vulkan_video_codecs_common.h

            MoltenVK/MoltenVK/include/vulkan/vk_icd.h
            MoltenVK/MoltenVK/include/vulkan/vk_layer.h
            MoltenVK/MoltenVK/include/vulkan/vk_platform.h
            MoltenVK/MoltenVK/include/vulkan/vulkan.cppm
            MoltenVK/MoltenVK/include/vulkan/vulkan.h
            MoltenVK/MoltenVK/include/vulkan/vulkan.hpp
            MoltenVK/MoltenVK/include/vulkan/vulkan_android.h
            MoltenVK/MoltenVK/include/vulkan/vulkan_beta.h
            MoltenVK/MoltenVK/include/vulkan/vulkan_core.h
            MoltenVK/MoltenVK/include/vulkan/vulkan_directfb.h
            MoltenVK/MoltenVK/include/vulkan/vulkan_enums.hpp
            MoltenVK/MoltenVK/include/vulkan/vulkan_extension_inspection.hpp
            MoltenVK/MoltenVK/include/vulkan/vulkan_format_traits.hpp
            MoltenVK/MoltenVK/include/vulkan/vulkan_fuchsia.h
            MoltenVK/MoltenVK/include/vulkan/vulkan_funcs.hpp
            MoltenVK/MoltenVK/include/vulkan/vulkan_ggp.h
            MoltenVK/MoltenVK/include/vulkan/vulkan_handles.hpp
            MoltenVK/MoltenVK/include/vulkan/vulkan_hash.hpp
            MoltenVK/MoltenVK/include/vulkan/vulkan_hpp_macros.hpp
            MoltenVK/MoltenVK/include/vulkan/vulkan_ios.h
            MoltenVK/MoltenVK/include/vulkan/vulkan_macos.h
            MoltenVK/MoltenVK/include/vulkan/vulkan_metal.h
            MoltenVK/MoltenVK/include/vulkan/vulkan_raii.hpp
            MoltenVK/MoltenVK/include/vulkan/vulkan_screen.h
            MoltenVK/MoltenVK/include/vulkan/vulkan_shared.hpp
            MoltenVK/MoltenVK/include/vulkan/vulkan_static_assertions.hpp
            MoltenVK/MoltenVK/include/vulkan/vulkan_structs.hpp
            MoltenVK/MoltenVK/include/vulkan/vulkan_to_string.hpp
            MoltenVK/MoltenVK/include/vulkan/vulkan_vi.h
            MoltenVK/MoltenVK/include/vulkan/vulkan_video.hpp
            MoltenVK/MoltenVK/include/vulkan/vulkan_wayland.h
            MoltenVK/MoltenVK/include/vulkan/vulkan_win32.h
            MoltenVK/MoltenVK/include/vulkan/vulkan_xcb.h
            MoltenVK/MoltenVK/include/vulkan/vulkan_xlib.h
            MoltenVK/MoltenVK/include/vulkan/vulkan_xlib_xrandr.h

            MoltenVK/MoltenVK/MoltenVK/API/mvk_config.h
            MoltenVK/MoltenVK/MoltenVK/API/mvk_datatypes.h
            MoltenVK/MoltenVK/MoltenVK/API/mvk_config.h
            MoltenVK/MoltenVK/MoltenVK/API/mvk_deprecated_api.h
            MoltenVK/MoltenVK/MoltenVK/API/mvk_private_api.h
            MoltenVK/MoltenVK/MoltenVK/API/mvk_vulkan.h
            MoltenVK/MoltenVK/MoltenVK/API/vk_mvk_moltenvk.h

            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCmdDebug.h
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCmdDebug.mm
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCmdDispatch.h
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCmdDispatch.mm
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCmdDraw.h
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCmdDraw.mm
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCmdPipeline.h
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCmdPipeline.mm
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCmdQueries.h
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCmdQueries.mm
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCmdRendering.h
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCmdRendering.mm
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCmdTransfer.h
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCmdTransfer.mm
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCommand.h
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCommandBuffer.h
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCommandBuffer.mm
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCommandEncoderState.h
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCommandEncoderState.mm
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCommandEncodingPool.h
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCommandEncodingPool.mm
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCommandPipelineStateFactoryShaderSource.h
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCommandPool.h
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCommandPool.mm
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCommandResourceFactory.h
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCommandResourceFactory.mm
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKCommandTypePools.def
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKMTLBufferAllocation.h
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKMTLBufferAllocation.mm
            MoltenVK/MoltenVK/MoltenVK/Commands/MVKMTLResourceBindings.h

            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKBuffer.h
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKBuffer.mm
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKDescriptor.h
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKDescriptor.mm
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKDescriptorSet.h
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKDescriptorSet.mm
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKDevice.h
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKDevice.mm
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKDeviceFeatureStructs.def
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKDeviceMemory.h
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKDeviceMemory.mm
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKFramebuffer.h
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKFramebuffer.mm
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKImage.h
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKImage.mm
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKInstance.h
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKInstance.mm
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKPipeline.h
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKPipeline.mm
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKPixelFormats.h
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKPixelFormats.mm
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKQueryPool.h
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKQueryPool.mm
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKQueue.h
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKQueue.mm
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKRenderPass.h
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKRenderPass.mm
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKResource.h
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKResource.mm
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKShaderModule.h
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKShaderModule.mm
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKSurface.h
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKSurface.mm
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKSwapchain.h
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKSwapchain.mm
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKSync.h
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKSync.mm
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKVulkanAPIObject.h
            MoltenVK/MoltenVK/MoltenVK/GPUObjects/MVKVulkanAPIObject.mm

            MoltenVK/MoltenVK/MoltenVK/Layers/MVKExtensions.def
            MoltenVK/MoltenVK/MoltenVK/Layers/MVKExtensions.h
            MoltenVK/MoltenVK/MoltenVK/Layers/MVKExtensions.mm
            MoltenVK/MoltenVK/MoltenVK/Layers/MVKLayers.h
            MoltenVK/MoltenVK/MoltenVK/Layers/MVKLayers.mm

            MoltenVK/MoltenVK/MoltenVK/OS/CAMetalLayer+MoltenVK.h
            MoltenVK/MoltenVK/MoltenVK/OS/CAMetalLayer+MoltenVK.mm
            MoltenVK/MoltenVK/MoltenVK/OS/MTLRenderPassDepthAttachmentDescriptor+MoltenVK.h
            MoltenVK/MoltenVK/MoltenVK/OS/MTLRenderPassDepthAttachmentDescriptor+MoltenVK.m
            MoltenVK/MoltenVK/MoltenVK/OS/MTLRenderPassDescriptor+MoltenVK.h
            MoltenVK/MoltenVK/MoltenVK/OS/MTLRenderPassDescriptor+MoltenVK.m
            MoltenVK/MoltenVK/MoltenVK/OS/MTLRenderPassStencilAttachmentDescriptor+MoltenVK.h
            MoltenVK/MoltenVK/MoltenVK/OS/MTLRenderPassStencilAttachmentDescriptor+MoltenVK.m
            MoltenVK/MoltenVK/MoltenVK/OS/MTLRenderPipelineColorAttachmentDescriptor+MoltenVK.h
            MoltenVK/MoltenVK/MoltenVK/OS/MTLRenderPipelineColorAttachmentDescriptor+MoltenVK.m
            MoltenVK/MoltenVK/MoltenVK/OS/MTLRenderPipelineDescriptor+MoltenVK.h
            MoltenVK/MoltenVK/MoltenVK/OS/MTLRenderPipelineDescriptor+MoltenVK.m
            MoltenVK/MoltenVK/MoltenVK/OS/MTLSamplerDescriptor+MoltenVK.h
            MoltenVK/MoltenVK/MoltenVK/OS/MTLSamplerDescriptor+MoltenVK.m
            MoltenVK/MoltenVK/MoltenVK/OS/MTLTextureDescriptor+MoltenVK.h
            MoltenVK/MoltenVK/MoltenVK/OS/MTLTextureDescriptor+MoltenVK.m
            MoltenVK/MoltenVK/MoltenVK/OS/MVKGPUCapture.h
            MoltenVK/MoltenVK/MoltenVK/OS/MVKGPUCapture.mm
            MoltenVK/MoltenVK/MoltenVK/OS/NSString+MoltenVK.h
            MoltenVK/MoltenVK/MoltenVK/OS/NSString+MoltenVK.mm

            MoltenVK/MoltenVK/MoltenVK/Utility/MVKBaseObject.h
            MoltenVK/MoltenVK/MoltenVK/Utility/MVKBaseObject.mm
            MoltenVK/MoltenVK/MoltenVK/Utility/MVKBitArray.h
            MoltenVK/MoltenVK/MoltenVK/Utility/MVKBlockObserver.h
            MoltenVK/MoltenVK/MoltenVK/Utility/MVKBlockObserver.m
            MoltenVK/MoltenVK/MoltenVK/Utility/MVKCodec.h
            MoltenVK/MoltenVK/MoltenVK/Utility/MVKCodec.mm
            MoltenVK/MoltenVK/MoltenVK/Utility/MVKConfigMembers.def
            MoltenVK/MoltenVK/MoltenVK/Utility/MVKDXTnCodec.def
            MoltenVK/MoltenVK/MoltenVK/Utility/MVKEnvironment.cpp
            MoltenVK/MoltenVK/MoltenVK/Utility/MVKEnvironment.h
            MoltenVK/MoltenVK/MoltenVK/Utility/MVKFoundation.cpp
            MoltenVK/MoltenVK/MoltenVK/Utility/MVKFoundation.h
            MoltenVK/MoltenVK/MoltenVK/Utility/MVKInflectionMap.h
            MoltenVK/MoltenVK/MoltenVK/Utility/MVKLogging.h
            MoltenVK/MoltenVK/MoltenVK/Utility/MVKObjectPool.h
            MoltenVK/MoltenVK/MoltenVK/Utility/MVKSmallVector.h
            MoltenVK/MoltenVK/MoltenVK/Utility/MVKSmallVectorAllocator.h
            MoltenVK/MoltenVK/MoltenVK/Utility/MVKWatermark.h
            MoltenVK/MoltenVK/MoltenVK/Utility/MVKWatermark.mm
            MoltenVK/MoltenVK/MoltenVK/Utility/MVKWatermarkShaderSource.h
            MoltenVK/MoltenVK/MoltenVK/Utility/MVKWatermarkTextureContent.h

            MoltenVK/MoltenVK/MoltenVK/Vulkan/mvk_api.mm
            MoltenVK/MoltenVK/MoltenVK/Vulkan/mvk_datatypes.hpp
            MoltenVK/MoltenVK/MoltenVK/Vulkan/mvk_datatypes.mm
            MoltenVK/MoltenVK/MoltenVK/Vulkan/vulkan.mm
    )

    target_include_directories(MoltenVK PUBLIC
            MoltenVK/MoltenVK/MoltenVK/API
            MoltenVK/MoltenVK/MoltenVK/Commands
            MoltenVK/MoltenVK/MoltenVK/GPUObjects
            MoltenVK/MoltenVK/MoltenVK/Layers
            MoltenVK/MoltenVK/MoltenVK/OS
            MoltenVK/MoltenVK/MoltenVK/Utility
            MoltenVK/MoltenVK/MoltenVK/Vulkan
            MoltenVK/Common
            ${EXTERNAL}/Vulkan-Headers/include
            ${EXTERNAL}/SPIRV-Cross
            "/Users/$ENV{USER}/VulkanSDK/1.4.309.0/macOS/include"
            #TODO: Make sure moltenVK is working fine.
    )
endif ()