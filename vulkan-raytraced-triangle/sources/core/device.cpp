#include "core/settings.hpp"
#include "core/device.hpp"
#include "core/app.hpp"

#include <iostream>
#include <cstring>
#include <vector>
using std::cout, std::endl;
using std::vector;

constexpr uint32 QUEUE_FAMILY_NOT_FOUND = ~0U;

/* Return
 *   모든 extension을 지원하면 true
 *   모든 extension을 지원하지 않으면 false
*/
bool isDeviceExtensionsSupport(VkPhysicalDevice pDevice) {
    unsigned int extensionPropsCount{ };
    vkEnumerateDeviceExtensionProperties(pDevice, nullptr, &extensionPropsCount, nullptr);

    vector<VkExtensionProperties> extensionProps(extensionPropsCount);
    vkEnumerateDeviceExtensionProperties(pDevice, nullptr, &extensionPropsCount, extensionProps.data());

    for (uint32 i = 0; i < (sizeof(Settings::Extension::DEVICE) / sizeof(Settings::Extension::DEVICE[0])); ++i) {
        const char* extension = Settings::Extension::DEVICE[i];

        bool isExist = { };
        for (const auto& support: extensionProps) {
            if (strcmp(extension, support.extensionName) == 0) {
                isExist = true;
                break;
            }
        }

        if (!isExist)
            return false;
    }

    return true;
}
/* Return
 *   모든 extension을 지원하는 physical device를 찾으면 그 physical device의 handle
 *   모든 extension을 지원하는 physical device를 찾지 못했으면 nullptr
*/
VkPhysicalDevice findPhysicalDevice() {
    unsigned int pDeviceCount{ };
    vkEnumeratePhysicalDevices(App::instance(), &pDeviceCount, nullptr);

    vector<VkPhysicalDevice> pDevices(pDeviceCount);
    vkEnumeratePhysicalDevices(App::instance(), &pDeviceCount, pDevices.data());

    for (const auto& pDevice: pDevices) {
        if (isDeviceExtensionsSupport(pDevice)) {
            // debug
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(pDevice, &props);
            cout << "Selected Device: " << props.deviceName << endl;
            cout << "Device Vulkan API: " << props.apiVersion << endl;

            return pDevice;
        }
    }

    return nullptr;
}
/* Return
 *   physical device의 queue family들 중에서 surface present를 지원하면서,
 *   그래픽 연산을 수행할 수 있는 queue family를 찾았다면 그 queue family의 index
 *
 *   지원되는 queue family가 없다면 QUEUE_FAMILY_NOT_FOUND (~0U)
*/
unsigned int findQueueFamily(VkPhysicalDevice pDevice) {
    unsigned int qFamilyPropsCount{ };
    vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &qFamilyPropsCount, nullptr);

    vector<VkQueueFamilyProperties> qFamilyProps(qFamilyPropsCount);
    vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &qFamilyPropsCount, qFamilyProps.data());

    for (unsigned int qFamilyIdx{ }; qFamilyIdx < qFamilyPropsCount; ++qFamilyIdx) {
        VkBool32 surfacePresentSupport{ };
        vkGetPhysicalDeviceSurfaceSupportKHR(pDevice, qFamilyIdx, App::window(), &surfacePresentSupport);

        if ((qFamilyProps[qFamilyIdx].queueFlags & VK_QUEUE_GRAPHICS_BIT) and surfacePresentSupport)
            return qFamilyIdx;
    }

    return QUEUE_FAMILY_NOT_FOUND;
}

/* -------- Constructors & Destructor -------- */
Device::Device() { }
Device::~Device() noexcept { vkDestroyDevice(mLogicalHandle, nullptr); }
/* ------------------------------------------- */

/* ---------- Operator Overloadings ---------- */
Device::operator bool() const noexcept { return mIsCreated; }
Device::operator VkPhysicalDevice() const noexcept { return mPhysicalHandle; }
Device::operator VkDevice() const noexcept { return mLogicalHandle; }
/* ------------------------------------------- */

/* Return
 *   device 생성 성공시 true
 *   device 생성 실패시 false
*/
bool Device::create() {
    if (!mIsCreated) {
        mPhysicalHandle = findPhysicalDevice();
        if (mPhysicalHandle == nullptr)
            cout << "[ERROR]: Physical Device Not Found" << endl;

        else {
            mQueueFamilyIdx = findQueueFamily(mPhysicalHandle);
            if (mQueueFamilyIdx == QUEUE_FAMILY_NOT_FOUND)
                cout << "[ERROR]: Queue Family Not Found" << endl;

            else {
                float qPriorities{ 1.0f };

                VkDeviceQueueCreateInfo qInfo {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .queueFamilyIndex = mQueueFamilyIdx,
                    .queueCount = 1,
                    .pQueuePriorities = &qPriorities
                };

                VkDeviceCreateInfo lDeviceInfo {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                    .queueCreateInfoCount = 1,
                    .pQueueCreateInfos = &qInfo,
                    .enabledExtensionCount = static_cast<uint32>(sizeof(Settings::Extension::DEVICE) / sizeof(Settings::Extension::DEVICE[0])),
                    .ppEnabledExtensionNames = Settings::Extension::DEVICE,
                };

                VkPhysicalDeviceFeatures deviceFeatures {
                    .samplerAnisotropy = VK_TRUE
                };
                lDeviceInfo.pEnabledFeatures = &deviceFeatures;

                // AS 에서 필요한 features
                VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
                    .bufferDeviceAddress = VK_TRUE
                };
                VkPhysicalDeviceAccelerationStructureFeaturesKHR asFeatures {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
                    .accelerationStructure = VK_TRUE
                };
                VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
                    .rayTracingPipeline = VK_TRUE
                };

                lDeviceInfo.pNext = &bufferDeviceAddressFeatures;
                bufferDeviceAddressFeatures.pNext = &asFeatures;
                asFeatures.pNext = &rtPipelineFeatures;

                if (vkCreateDevice(mPhysicalHandle, &lDeviceInfo, nullptr, &mLogicalHandle) == VK_SUCCESS) {
                    vkGetDeviceQueue(mLogicalHandle, mQueueFamilyIdx, 0, &mQueue);

                    mIsCreated = true;
                }
            }
        }
    }

    return mIsCreated;
}

/* ----------------- Getters ----------------- */
VkQueue Device::queue() const noexcept { return mQueue; }
uint32  Device::queueFamilyIndex() const noexcept { return mQueueFamilyIdx; }
/* ------------------------------------------- */