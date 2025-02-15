#define GLFW_INCLUDE_VULKAN
#include "glfw/glfw3.h"

#include "core/window.hpp"

#include <iostream>
using std::cout, std::endl;

bool Window::isGLFWInitialized = { };

Window::Window() { }
Window::Window(const unsigned int& w, const unsigned int& h, const string& title)
    : mWidth{w}, mHeight{h}, mTitle{title} { init(w, h, title); }
Window::~Window() noexcept { if (isInitialized) glfwDestroyWindow(mHandle); }

void Window::init(const unsigned int& w, const unsigned int& h, const string& title) {
    if (!isGLFWInitialized) {
        if (!glfwInit())
            cout << "[ERROR]: GLFW INIT" << endl;

        isGLFWInitialized = true;
    }

    if (!isInitialized) {
        glfwSetErrorCallback( [](int errCode, const char* desc) { cout << "[ERROR " << errCode << "]: " << desc << endl; } );
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        mHandle = glfwCreateWindow(static_cast<int>(w), static_cast<int>(h), title.c_str(), nullptr, nullptr);
    }
}

unsigned int Window::width() const { return mWidth; }
unsigned int Window::height() const { return mHeight; }

GLFWwindow* Window::handle() { return mHandle; }