#include <vk_nutshell.h>
#include <iostream>



void nutshell::beforeRender() {}


void nutshell::whileRendering() {
  //std::cout << "Hello, World!" << std::endl;
}
void nutshell::drawCallBackMain(GLFWwindow *pWindow, VkInstance *instance, VkDevice *device, VkQueue *queue) {

}


void nutshell::afterRedner() {}


int main() {


  std::cout << "Hello, World!" << std::endl << "Starting TeapotBasic" << std::endl;


  auto context = nutshell::VkContext();

  context.programLoop();

  return 0;
}
