#include "core/app.hpp"
#include "core/settings.hpp"

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

    if (!mInstance.create(Settings::App::VK_APP_NAME, Settings::App::VK_API_VER_1_3))
        cout << "[ERROR]: Instance Creation" << endl;

    if (!mWindow.createSurface(mInstance))
        cout << "[ERROR]: Surface Creation" << endl;

    if (!mDevice.create())
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