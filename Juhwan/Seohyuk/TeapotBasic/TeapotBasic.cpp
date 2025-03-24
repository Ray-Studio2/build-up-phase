#include "vk_nutshell.h"
#include "seohyuk.h"
#include <iostream>



void nutshell::beforeRender() {}

void nutshell::drawCallPreRender(GLFWwindow *pWindow, vk::Instance instance, vk::Device device, vk::Queue queue) {}

void nutshell::whileRendering() {
  //std::cout << "Hello, World!" << std::endl;
}

void nutshell::drawCallBackMain(GLFWwindow *pWindow, vk::Instance instance, vk::Device device, vk::Queue queue) {}

void nutshell::drawCallPostRender(GLFWwindow *pWindow, vk::Instance instance, vk::Device device, vk::Queue queue) {}

void nutshell::afterRedner() {}


int main() {


  std::cout << "Hello, World!" << std::endl << "Starting TeapotBasic" << std::endl;

  auto context = nutshell::VkContext();
  const auto window = spawnWindow(800, 640);

  context.injectGLFWWindow(window);


  context.programLoop();

  glfwDestroyWindow(window);

  return 0;
}
