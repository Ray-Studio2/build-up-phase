#include "core/app.hpp"

#include <iostream>
using std::cout, std::endl;

/* -------- static variables init -------- */
Instance App::mInstance;
Window   App::mWindow;
Device   App::mDevice;
/* --------------------------------------- */

void App::init(uint32 width, uint32 height, const char* title) {
    if (!mWindow.createWindow(width, height, title))
        cout << "[ERROR]: Window Creation" << endl;

    if (!mInstance.create("Vulkan Base", VK_API_VERSION_1_2))
        cout << "[ERROR]: Instance Creation" << endl;

    if (!mWindow.createSurface(mInstance))
        cout << "[ERROR]: Surface Creation" << endl;

    vector<const char*> reqDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,

        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,

        VK_KHR_SPIRV_1_4_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
    };
    if (!mDevice.create(reqDeviceExtensions))
        cout << "[ERROR]: Device Creation" << endl;
}
void App::terminate() {
    vkDeviceWaitIdle(mDevice);
    glfwTerminate();
}

/* -------- Getters -------- */
Window&   App::window()   { return mWindow; }
Instance& App::instance() { return mInstance; }
Device&   App::device()   { return mDevice; }
/* ------------------------- */