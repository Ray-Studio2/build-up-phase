#include "Addon_imgui.h"
#include "ThirdParty/imgui/imgui.h"
#include "ThirdParty/imgui/imgui_impl_glfw.h"
#include "ThirdParty/imgui/imgui_impl_vulkan.h"
#include <GLFW/glfw3.h>
#include "Vulkan.h"

using namespace A3;

// Dear ImGui: standalone example application for Glfw + Vulkan

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// Important note to the reader who wish to integrate imgui_impl_vulkan.cpp/.h in their own engine/app.
// - Common ImGui_ImplVulkan_XXX functions and structures are used to interface with imgui_impl_vulkan.cpp/.h.
//   You will use those if you want to use this rendering backend in your engine/app.
// - Helper ImGui_ImplVulkanH_XXX functions and structures are only used by this example (main.cpp) and by
//   the backend itself (imgui_impl_vulkan.cpp), but should PROBABLY NOT be used by your own engine/app code.
// Read comments in imgui_impl_vulkan.h.

#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Volk headers
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
#define VOLK_IMPLEMENTATION
#include <volk.h>
#endif

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

//#define APP_USE_UNLIMITED_FRAME_RATE
#ifdef _DEBUG
#define APP_USE_VULKAN_DEBUG_REPORT
#endif

// Data
static VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
static VkPipelineCache          g_PipelineCache = VK_NULL_HANDLE;

static ImGui_ImplVulkanH_Window g_MainWindowData;
static uint32_t                 g_MinImageCount = 2;
static bool                     g_SwapChainRebuild = false;

static void check_vk_result( VkResult err )
{
    if( err == VK_SUCCESS )
        return;
    fprintf( stderr, "[vulkan] Error: VkResult = %d\n", err );
    if( err < 0 )
        abort();
}

void Addon_imgui::CleanupVulkan( VulkanRendererBackend* vulkan )
{
    vkDestroyDescriptorPool( vulkan->device, vulkan->descriptorPool, vulkan->allocator );

#ifdef APP_USE_VULKAN_DEBUG_REPORT
    // Remove the debug report callback
    auto f_vkDestroyDebugReportCallbackEXT = ( PFN_vkDestroyDebugReportCallbackEXT )vkGetInstanceProcAddr( vulkan->instance, "vkDestroyDebugReportCallbackEXT" );
    f_vkDestroyDebugReportCallbackEXT( vulkan->instance, g_DebugReport, vulkan->allocator );
#endif // APP_USE_VULKAN_DEBUG_REPORT

    vkDestroyDevice( vulkan->device, vulkan->allocator );
    vkDestroyInstance( vulkan->instance, vulkan->allocator );
}

void Addon_imgui::CleanupVulkanWindow( VulkanRendererBackend* vulkan )
{
    ImGui_ImplVulkanH_DestroyWindow( vulkan->instance, vulkan->device, &g_MainWindowData, vulkan->allocator );
}

static void FrameRender( VkDevice& device, VkQueue& gfxQueue, ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data )
{
    
}

Addon_imgui::Addon_imgui( GLFWwindow* window, VulkanRendererBackend* vulkan, int32 screenWidth, int32 screenHeight )
{
    // Create Framebuffers
    ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
    // @TODO: Replace from here
    wd->Surface = vulkan->surface;

    // Select Surface Format
    const VkSurfaceFormatKHR surfaceFormat
    {
        .format = VK_FORMAT_B8G8R8A8_SRGB,
        .colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR
    };
    wd->SurfaceFormat = surfaceFormat;
    wd->PresentMode = VK_PRESENT_MODE_MAILBOX_KHR;

    wd->Swapchain = vulkan->swapChain;
    wd->ImageCount = 3;

    wd->SemaphoreCount = wd->ImageCount;
    wd->Frames.resize( wd->ImageCount );
    wd->FrameSemaphores.resize( wd->SemaphoreCount );
    memset( wd->Frames.Data, 0, wd->Frames.size_in_bytes() );
    memset( wd->FrameSemaphores.Data, 0, wd->FrameSemaphores.size_in_bytes() );

    for( uint32_t i = 0; i < vulkan->swapChainImages.size(); i++ )
    {
        wd->Frames[ i ].Backbuffer = vulkan->swapChainImages[ i ];
        wd->Frames[ i ].BackbufferView = vulkan->swapChainImageViews[ i ];
        wd->Frames[ i ].Framebuffer = vulkan->framebuffers[ i ];
        wd->Frames[ i ].CommandPool = vulkan->commandPools[ i ];
        wd->Frames[ i ].CommandBuffer = vulkan->commandBuffers[ i ];
        wd->Frames[ i ].Fence = vulkan->fences[ i ];

        wd->FrameSemaphores[ i ].ImageAcquiredSemaphore = vulkan->imageAvailableSemaphores[ i ];
        wd->FrameSemaphores[ i ].RenderCompleteSemaphore = vulkan->renderFinishedSemaphores[ i ];
    }

    wd->RenderPass = vulkan->imguiRenderPass;

    wd->Width = screenWidth;
    wd->Height = screenHeight;
    // ~To here

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); ( void )io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
    {
        style.WindowRounding = 0.0f;
        style.Colors[ ImGuiCol_WindowBg ].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan( window, true );
    ImGui_ImplVulkan_InitInfo init_info = {};
    //init_info.ApiVersion = VK_API_VERSION_1_3;              // Pass in your value of VkApplicationInfo::apiVersion, otherwise will default to header version.
    init_info.Instance = vulkan->instance;
    init_info.PhysicalDevice = vulkan->physicalDevice;
    init_info.Device = vulkan->device;
    init_info.QueueFamily = vulkan->queueFamilyIndex;
    init_info.Queue = vulkan->graphicsQueue;
    init_info.PipelineCache = g_PipelineCache;
    init_info.DescriptorPool = vulkan->descriptorPool;
    init_info.RenderPass = wd->RenderPass;
    init_info.Subpass = 0;
    init_info.MinImageCount = g_MinImageCount;
    init_info.ImageCount = wd->ImageCount;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = vulkan->allocator;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init( &init_info );
}

void Addon_imgui::renderFrame( GLFWwindow* window, VulkanRendererBackend* vulkan )
{
    // Our state
    static bool show_demo_window = true;
    static bool show_another_window = false;
    static ImVec4 clear_color = ImVec4( 0.45f, 0.55f, 0.60f, 1.00f );

    ImGuiIO& io = ImGui::GetIO();

    // Resize swap chain?
    int fb_width, fb_height;
    glfwGetFramebufferSize( window, &fb_width, &fb_height );
    if( fb_width > 0 && fb_height > 0 && ( g_SwapChainRebuild || g_MainWindowData.Width != fb_width || g_MainWindowData.Height != fb_height ) )
    {
        ImGui_ImplVulkan_SetMinImageCount( g_MinImageCount );
        ImGui_ImplVulkanH_CreateOrResizeWindow( vulkan->instance, vulkan->physicalDevice, vulkan->device, &g_MainWindowData, vulkan->queueFamilyIndex, vulkan->allocator, fb_width, fb_height, g_MinImageCount );
        g_MainWindowData.FrameIndex = 0;
        g_SwapChainRebuild = false;
    }

    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if( show_demo_window )
        ImGui::ShowDemoWindow( &show_demo_window );

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin( "Hello, world!" );                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text( "This is some useful text." );               // Display some text (you can use a format strings too)
        ImGui::Checkbox( "Demo Window", &show_demo_window );      // Edit bools storing our window open/close state
        ImGui::Checkbox( "Another Window", &show_another_window );

        ImGui::SliderFloat( "float", &f, 0.0f, 1.0f );            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3( "clear color", ( float* )&clear_color ); // Edit 3 floats representing a color

        if( ImGui::Button( "Button" ) )                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text( "counter = %d", counter );

        ImGui::Text( "Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate );
        ImGui::End();
    }

    // 3. Show another simple window.
    if( show_another_window )
    {
        ImGui::Begin( "Another Window", &show_another_window );   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text( "Hello from another window!" );
        if( ImGui::Button( "Close Me" ) )
            show_another_window = false;
        ImGui::End();
    }

    // Rendering
    ImGui::Render();
    ImDrawData* main_draw_data = ImGui::GetDrawData();
    const bool main_is_minimized = ( main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f );
    ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
    wd->ClearValue.color.float32[ 0 ] = clear_color.x * clear_color.w;
    wd->ClearValue.color.float32[ 1 ] = clear_color.y * clear_color.w;
    wd->ClearValue.color.float32[ 2 ] = clear_color.z * clear_color.w;
    wd->ClearValue.color.float32[ 3 ] = clear_color.w;
    
    {
        VkSemaphore image_acquired_semaphore = wd->FrameSemaphores[ wd->SemaphoreIndex ].ImageAcquiredSemaphore;
        VkSemaphore render_complete_semaphore = wd->FrameSemaphores[ wd->SemaphoreIndex ].RenderCompleteSemaphore;
        VkResult err;

        ImGui_ImplVulkanH_Frame* fd = &wd->Frames[ wd->FrameIndex ];
        {
            err = vkResetCommandPool( vulkan->device, fd->CommandPool, 0 );
            check_vk_result( err );
            VkCommandBufferBeginInfo info{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            };
            err = vkBeginCommandBuffer( fd->CommandBuffer, &info );
            check_vk_result( err );
        }
        {
            VkClearValue cv{};
            cv.color = VkClearColorValue{ 0.5f, 0.5f, 1.0f, };

            VkRenderPassBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            info.renderPass = wd->RenderPass;
            info.framebuffer = fd->Framebuffer;
            info.renderArea.extent.width = wd->Width;
            info.renderArea.extent.height = wd->Height;
            info.clearValueCount = 1;
            info.pClearValues = &cv;
            vkCmdBeginRenderPass( fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE );
        }

        // Record dear imgui primitives into command buffer
        ImGui_ImplVulkan_RenderDrawData( main_draw_data, fd->CommandBuffer );

        // Submit command buffer
        vkCmdEndRenderPass( fd->CommandBuffer );
        {
            VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            info.waitSemaphoreCount = 1;
            info.pWaitSemaphores = &render_complete_semaphore;
            info.pWaitDstStageMask = &wait_stage;
            info.commandBufferCount = 1;
            info.pCommandBuffers = &fd->CommandBuffer;
            info.signalSemaphoreCount = 1;
            info.pSignalSemaphores = &render_complete_semaphore;

            err = vkEndCommandBuffer( fd->CommandBuffer );
            check_vk_result( err );
            err = vkQueueSubmit( vulkan->graphicsQueue, 1, &info, fd->Fence );
            check_vk_result( err );
        }

        wd->SemaphoreIndex = ( wd->SemaphoreIndex + 1 ) % wd->SemaphoreCount; // Now we can use the next set of semaphores
        wd->FrameIndex = ( wd->FrameIndex + 1 ) % 3; // @FIXME: Workaround
    }

    // Update and Render additional Platform Windows
    if( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}
