#include "core/window.hpp"

#include <iostream>
using std::cout, std::endl;

/* ------------ Static Variables ------------- */
bool Window::mIsGLFWInitialized = { };
/* ------------------------------------------- */

/* -------- Constructors & Destructor -------- */
Window::Window() { }
Window::Window(uint32 width, uint32 height, const char* title)
    : mWidth{width}, mHeight{height} { createWindow(width, height, title); }
Window::~Window() noexcept {
    vkDestroySurfaceKHR(instancePtr, mSurface, nullptr);
    glfwDestroyWindow(mHandle);
}
/* ------------------------------------------- */

/* ---------- Operator Overloadings ---------- */
Window::operator bool() const noexcept { return mIsWindowCreated; }
Window::operator GLFWwindow*() const noexcept { return mHandle; }
Window::operator VkSurfaceKHR() const noexcept { return mSurface; }
/* ------------------------------------------- */

/* Return
 *   window 생성 성공시 true
 *   window 생성 실패시 false
*/
bool Window::createWindow(uint32 width, uint32 height, const char* title) {
    if (!mIsGLFWInitialized) {
        if (glfwInit() != GLFW_TRUE)
            cout << "[ERROR]: glfwInit()" << endl;

        else
            mIsGLFWInitialized = true;
    }

    if (mIsGLFWInitialized and !mIsWindowCreated) {
        glfwSetErrorCallback( [](int errCode, const char* desc) { cout << "[ERROR " << errCode << "]: " << desc << endl; } );
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        mHandle = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title, nullptr, nullptr);

        if (mHandle != nullptr)
            mIsWindowCreated = true;
    }

    return mIsWindowCreated;
}
/* Return
 *   surface 생성 성공시 true
 *   surface 생성 실패시 false
*/
bool Window::createSurface(VkInstance instance) {
    if (mIsWindowCreated and !mIsSurfaceCreated) {
        if (glfwCreateWindowSurface(instance, mHandle, nullptr, &mSurface) == VK_SUCCESS) {
            instancePtr = instance;
            mIsSurfaceCreated = true;
        }
    }

    return mIsSurfaceCreated;
}

/* ----------------- Getters ----------------- */
Window::uint32 Window::width()  const noexcept { return mWidth; }
Window::uint32 Window::height() const noexcept { return mHeight; }
const char*    Window::title()  const { return glfwGetWindowTitle(mHandle); }
/* ------------------------------------------- */