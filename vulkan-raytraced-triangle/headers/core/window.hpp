#pragma once

#define GLFW_INCLUDE_VULKAN
#include "glfw3.h"
#include "vulkan/vulkan.h"

/* TODO
 *   - Input 시스템 구성 (Event 기반)
*/

class Window {
    Window(const Window&) = delete;
    Window(Window&&) noexcept = delete;

    Window& operator=(const Window&) = delete;
    Window& operator=(Window&&) noexcept = delete;

    private:
        using uint32 = unsigned int;

    public:
        Window();
        Window(uint32 width, uint32 height, const char* title);
        ~Window() noexcept;

        explicit operator bool() const noexcept;
        operator GLFWwindow*() const noexcept;
        operator VkSurfaceKHR() const noexcept;

        bool createWindow(uint32 width, uint32 height, const char* title);
        bool createSurface(VkInstance instance);

        uint32 width()  const noexcept;
        uint32 height() const noexcept;
        const char* title() const;

    private:
        bool mIsWindowCreated{ };
        bool mIsSurfaceCreated{ };

        uint32 mWidth{ };
        uint32 mHeight{ };

        GLFWwindow* mHandle{ };
        VkSurfaceKHR mSurface{ };
        VkInstance instancePtr{ };

        static bool mIsGLFWInitialized;
};