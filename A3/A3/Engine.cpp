#include "Engine.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <filesystem>
#include <tuple>
#include <bitset>
#include <span>
#include "Vulkan.h"
#include "PathTracingRenderer.h"
#include "Addon_imgui.h"
#include "Scene.h"

namespace A3
{
GLFWwindow* createWindow()
{
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );
    return glfwCreateWindow( RenderSettings::screenWidth, RenderSettings::screenHeight, "Vulkan", nullptr, nullptr );
}

static void glfw_error_callback( int error, const char* description )
{
    fprintf( stderr, "GLFW Error %d: %s\n", error, description );
}

void Engine::Run()
{
    glfwSetErrorCallback( glfw_error_callback );
    glfwInit();

    // Create window with Vulkan context
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    //GLFWwindow* window = glfwCreateWindow( 1280, 720, "Dear ImGui GLFW+Vulkan example", nullptr, nullptr );
    if( !glfwVulkanSupported() )
    {
        printf( "GLFW: Vulkan Not Supported\n" );
    }

    std::vector<const char*> extensions;
    uint32_t extensions_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions( &extensions_count );
    for( uint32_t i = 0; i < extensions_count; i++ )
        extensions.push_back( glfw_extensions[ i ] );
    if( ON_DEBUG ) extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );

    GLFWwindow* window = createWindow();

    int32 screenWidth, screenHeight;
    glfwGetFramebufferSize( window, &screenWidth, &screenHeight );

    {
        VulkanRendererBackend gfxBackend( window, extensions, screenWidth, screenHeight );

        Scene scene;

        PathTracingRenderer renderer( &gfxBackend );

        Addon_imgui imgui( window, &gfxBackend, screenWidth, screenHeight );

        while( !glfwWindowShouldClose( window ) )
        {
            glfwPollEvents();

            scene.beginFrame();

            renderer.beginFrame( screenWidth, screenHeight );

            renderer.render( scene );
            imgui.renderFrame( window, &gfxBackend );

            renderer.endFrame();

            scene.endFrame();
        }
    }

    glfwDestroyWindow( window );
    glfwTerminate();
}
}