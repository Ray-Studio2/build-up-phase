#pragma once

#include "core/window.hpp"

class App {
    App() = delete;
    App(const App&) = delete;
    App(App&&) noexcept = delete;
    ~App() noexcept = delete;

    App& operator=(const App&) = delete;
    App& operator=(App&&) noexcept = delete;

    public:
        static void init(const unsigned int&, const unsigned int&, const string&);

        static Window* activeWindow();

    private:
        static Window* mActiveWindow;
};