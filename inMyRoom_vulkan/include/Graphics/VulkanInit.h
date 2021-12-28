#pragma once

#include <string>
#include <map>

#include "configuru.hpp"
#include "vulkan/vulkan.hpp"
#include "vk_mem_alloc.hpp"

#include "WindowWithAsyncInput.h"

struct QueuesList
{
    std::vector<std::pair<vk::Queue, uint32_t>> graphicsQueues;
    std::vector<std::pair<vk::Queue, uint32_t>> dedicatedComputeQueues;
    std::vector<std::pair<vk::Queue, uint32_t>> dedicatedTransferQueues;
};

class VulkanInit
{
public:
    explicit VulkanInit(const configuru::Config& in_cfgFile,
                        std::string appName = "inMyRoom_vulkan");
    ~VulkanInit();
    VulkanInit (const VulkanInit&) = delete;
    VulkanInit& operator= (const VulkanInit&) = delete;

    WindowWithAsyncInput* GetWindowPtr() const {return windowAsync_uptr.get();}

    vk::Device GetDevice() const {return device;}
    QueuesList GetQueuesList() const {return queues;}
    vk::SwapchainKHR GetSwapchain() const {return swapchain;}
    vk::SwapchainCreateInfoKHR GetSwapchainCreateInfo() const {return swapchainCreateInfo;}
    std::vector<vk::ImageView> GetSwapchainImageViews() const {return swapchainImageViews;}
    size_t GetSwapchainSize() const {return swapchainImageViews.size();}

    vma::Allocator GetVMAallocator() const {return vma_allocator;}

private:
    void CreateInstance(const std::string&              appName,
                        const std::string&              engineName,
                        const std::vector<std::string>& layers,
                        const std::vector<std::string>& extensions,
                        uint32_t                        apiVersion,
                        bool                            enableDebugMessenger);

    void CreateDevice(vk::PhysicalDevice physical_device,
                      uint32_t graphics_count,
                      uint32_t dedicated_compute_count,
                      uint32_t dedicated_transfer_count,
                      const std::vector<std::string>& extensions,
                      const vk::PhysicalDeviceFeatures& features,
                      void* create_device_pNext = nullptr);

    void InitializeVMA(vma::AllocatorCreateFlags allocator_flags,
                       uint32_t                  apiVersion);

    void CreateWindow_(const std::string& title,
                       uint32_t width,
                       uint32_t height);

    void CreateSwapchain(vk::SurfaceFormatKHR surface_format,
                         vk::PresentModeKHR present_mode,
                         uint32_t swapchain_count,
                         vk::ImageUsageFlags usage);

protected:
    vk::Instance                    vulkanInstance;
    vk::DebugUtilsMessengerEXT      vulkanDebugUtilsMessenger;
    vk::Device                      device;
    vk::PhysicalDevice              physicalDevice;
    QueuesList                      queues;
    vk::SurfaceKHR                  surface;
    vk::SwapchainKHR                swapchain;
    std::vector<vk::ImageView>      swapchainImageViews;

    vk::SwapchainCreateInfoKHR swapchainCreateInfo;

    vma::Allocator                  vma_allocator;
    std::unique_ptr<vma::VulkanFunctions> vma_vulkanFunctions_uptr;

    const configuru::Config& cfgFile;

    std::map<std::string, vk::PresentModeKHR> presentAliasToPresentMode_map =
    {
        {"Immediate", vk::PresentModeKHR::eImmediate},
        {"FIFO", vk::PresentModeKHR::eFifo},
        {"FIFOrelaxed", vk::PresentModeKHR::eFifoRelaxed},
        {"Mailbox", vk::PresentModeKHR::eMailbox}
    };

    std::vector<vk::SurfaceFormatKHR> preferredSurfaceFormatOrder =
    {
        {vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear},
        {vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear},
        {vk::Format::eB8G8R8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear},
        {vk::Format::eR8G8B8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear}
    };

    std::unique_ptr<WindowWithAsyncInput> windowAsync_uptr;

    const uint32_t                   windowHeight;
    const uint32_t                   windowWidth;
};

VKAPI_ATTR VkBool32 VKAPI_CALL
debugUtilsMessengerCallback( VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
                             VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
                             VkDebugUtilsMessengerCallbackDataEXT const * pCallbackData,
                             void * /*pUserData*/ );