#include "core/app.hpp"

Window* App::mActiveWindow = { };

void App::init(const unsigned int& w, const unsigned int& h, const string& title) {
    mActiveWindow = new Window(w, h, title);
}

Window* App::activeWindow() { return mActiveWindow; }