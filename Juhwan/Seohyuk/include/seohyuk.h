//
// Created by nefty on 2025-03-22.
//

#pragma once

#include <iostream>
#include <ostream>
#include <utility>
#include <GLFW/glfw3.h>


#define WINDOW_NAME "TeapotBasic"

GLFWwindow * spawnWindow(const int width, const int height) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow * window = glfwCreateWindow(width, height, WINDOW_NAME,  NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);

    return window;
}
