#pragma once

#include "core/types.hpp"
#include "core/window.hpp"
#include "core/instance.hpp"
#include "core/device.hpp"

/* TODO
 *   - glfwTerminate() 선호출 문제 해결
 *     - idea 1. static window count 사용. count == 1 에서 소멸자 호출되면 glfwTerminate 호출
 *       * static화 했던거 뺄 생각도 해보자
 *
 *   - DEBUG 빌드에서만 동작하는 Error Handler("message", errorCode) 배치
*/

class App {
    App() = delete;
    App(const App&) = delete;
    App(App&&) noexcept = delete;
    ~App() noexcept = delete;

    App& operator=(const App&) = delete;
    App& operator=(App&&) noexcept = delete;

    public:
        static void init(uint32 width, uint32 height, const char* title);
        static void terminate();

        static Instance& instance();
        static Window&   window();
        static Device&   device();

    private:
        static Instance mInstance;
        static Window   mWindow;
        static Device   mDevice;
};