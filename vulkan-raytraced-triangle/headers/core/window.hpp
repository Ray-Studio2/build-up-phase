#pragma once

#include <string>
using std::string;

struct GLFWwindow;

class Window {
    public:
        Window();
        Window(const unsigned int&, const unsigned int&, const string&);
        ~Window() noexcept;

        void init(const unsigned int&, const unsigned int&, const string&);

        unsigned int width() const;
        unsigned int height() const;

        GLFWwindow* handle();

    private:
        static bool isGLFWInitialized;
        bool isInitialized{ };

        unsigned int mWidth{ };
        unsigned int mHeight{ };

        string mTitle;

        GLFWwindow* mHandle{ };
};