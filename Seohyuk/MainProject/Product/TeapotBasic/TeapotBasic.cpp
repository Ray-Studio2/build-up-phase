#include <vk_nutshell.h>
#include <iostream>





void nutshell::whileRendering() {
  //std::cout << "Hello, World!" << std::endl;
}
void nutshell::drawCallBackMain(GLFWwindow *pWindow, VkInstance instance, VkDevice device, VkQueue queue, VkCommandBuffer commandBuffer, Synchronizer synchronizer) {
  vkWaitForFences(device, 1, &synchronizer.fence, VK_TRUE, UINT64_MAX);


  VkCommandBufferBeginInfo commandBeginInfo = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    nullptr,
    0,
    nullptr
  };





  vkResetFences(device, 1, &synchronizer.fence);


  VkSubmitInfo submitInfo = {
  VK_STRUCTURE_TYPE_SUBMIT_INFO,
  nullptr,
    //pWaitSemaphores
    //pWaitDstStageMask
    //commandBufferCount
    //pCommandBuffers
    //signalSemaphoreCount
    //pSignalSemaphores
  };
  vkQueueSubmit(queue, 1, &submitInfo, reinterpret_cast<VkFence>(&synchronizer.fence));
}

int main() {
  std::cout << "Hello, World!" << std::endl << "Starting TeapotBasic" << std::endl;


  auto context = nutshell::VkContext();
  //auto instruction = nutshell::RenderingInstruction();

  context.programLoop();

  return 0;
}
