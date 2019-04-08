#pragma once

#include <map>

#include "configuru.hpp"

#include "config.h"
#include "wrappers/instance.h"
#include "wrappers/device.h"
#include "wrappers/rendering_surface.h"
#include "misc/window_factory.h"
#include "misc/swapchain_create_info.h"
#include "misc/rendering_surface_create_info.h"

#include "WindowWithAsyncInput.h"
#include "CameraBaseClass.h"

class VulkanInit
{
public:
    VulkanInit(const configuru::Config& in_cfgFile);
    ~VulkanInit();

    Anvil::InstanceUniquePtr         instance_uptr;
    Anvil::BaseDeviceUniquePtr       device_uptr;
    WindowWithAsyncInputUniquePtr    windowAsync_uptr;
    Anvil::SwapchainUniquePtr        swapchain_uptr;

    const uint32_t                   swapchainImagesCount;
    const uint32_t                   windowHeight;
    const uint32_t                   windowWidth;

private:
    void InitVulkan();
    void InitWindow();
    void InitSwapchain();

    void OnValidationCallback(Anvil::DebugMessageSeverityFlags in_severity,
                              const char*                      in_message_ptr);

    Anvil::RenderingSurfaceUniquePtr renderingSurface_uptr;

    const configuru::Config& cfgFile;

    std::map<std::string, Anvil::PresentModeKHR> presentAliasToPresentMode_map =
    {
        {"Immediate", Anvil::PresentModeKHR::IMMEDIATE_KHR},
        {"FIFO", Anvil::PresentModeKHR::FIFO_KHR},
        {"FIFOrelaxed", Anvil::PresentModeKHR::FIFO_RELAXED_KHR},
        {"Mailbox", Anvil::PresentModeKHR::MAILBOX_KHR}
    };
};