#include "vk_nutshell.h"
#include "seohyuk.h"
#include <iostream>

int main() {


  std::cout << "Hello, World!" << std::endl << "Starting TeapotBasic" << std::endl;

  auto context = nutshell::VkContext();
  auto window = spawnWindow(800, 640);

  context.injectGLFWWindow(window);


  context.programLoop();

  glfwDestroyWindow(window);

  return 0;
}
