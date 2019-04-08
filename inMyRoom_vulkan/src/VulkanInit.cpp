#include "VulkanInit.h"

#include <iostream>

VulkanInit::VulkanInit(const configuru::Config& in_cfgFile)
    :
    cfgFile(in_cfgFile),
    swapchainImagesCount(in_cfgFile["graphicsSettings"]["buffersCount"].as_integer<unsigned int>()),
    windowWidth(in_cfgFile["graphicsSettings"]["xRes"].as_integer<unsigned int>()),
    windowHeight(in_cfgFile["graphicsSettings"]["yRes"].as_integer<unsigned int>())
{
    InitVulkan();
    InitWindow();
    InitSwapchain();
}

VulkanInit::~VulkanInit()
{
    Anvil::Vulkan::vkDeviceWaitIdle(device_uptr->get_device_vk());

    swapchain_uptr.reset();
    renderingSurface_uptr.reset();
    windowAsync_uptr.reset();
    device_uptr.reset();
    instance_uptr.reset();
}

void VulkanInit::InitVulkan()
{
    {
        /* Create a Vulkan instance */
        auto create_info_ptr = Anvil::InstanceCreateInfo::create("inMyRoom_vulkan", /* app_name */
                                                                 "inMyRoom_vulkan", /* engine_name */
#ifdef ENABLE_VALIDATION
                                                                 std::bind(&Graphics::on_validation_callback,
                                                                           this,
                                                                           std::placeholders::_1,
                                                                           std::placeholders::_2
                                                                 ),
#else
                                                                 Anvil::DebugCallbackFunction(),
#endif
                                                                 false); /* in_mt_safe */

        instance_uptr = Anvil::Instance::create(std::move(create_info_ptr));
    }


    {
        /* Determine which extensions we need to request for */
        std::vector<std::string> vulkan_layers;

        /* Create a Vulkan device */
        auto create_info_ptr = Anvil::DeviceCreateInfo::create_sgpu(instance_uptr->get_physical_device(0),
                                                                    true, /* in_enable_shader_module_cache */
                                                                    Anvil::DeviceExtensionConfiguration(),
                                                                    vulkan_layers, /* in_layers                               */
                                                                    Anvil::CommandPoolCreateFlagBits::NONE, /* in_transient_command_buffer_allocs_only */
                                                                    false); /* in_support_resettable_command_buffers   */

        device_uptr = Anvil::SGPUDevice::create(std::move(create_info_ptr));
    }
}

void VulkanInit::InitWindow()
{
#ifdef _WIN32
    const Anvil::WindowPlatform platform = Anvil::WINDOW_PLATFORM_SYSTEM;
#else
    const Anvil::WindowPlatform platform = Anvil::WINDOW_PLATFORM_XCB;
#endif

    windowAsync_uptr = std::make_unique<WindowWithAsyncInput>(platform,
                                                              "inMyRoom_vulkan",
                                                              windowWidth,
                                                              windowHeight,
                                                              true);
}

void VulkanInit::InitSwapchain()
{
    Anvil::Format swapchain_format(Anvil::Format::B8G8R8A8_UNORM);
    Anvil::PresentModeKHR swapchain_present_mode;
    {
        auto search = presentAliasToPresentMode_map.find(cfgFile["graphicsSettings"]["presentMode"].as_string());

        if(search != presentAliasToPresentMode_map.end())
        {
            swapchain_present_mode = search->second;
        }
        else
        {
            std::cout << "The present mode does not exist, fall back to FIFO/n";
            swapchain_present_mode = Anvil::PresentModeKHR::FIFO_KHR;
        }
    }
    
    Anvil::ImageUsageFlags swapchain_usage(
        Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT | Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT);

    {
        auto create_info_ptr = Anvil::RenderingSurfaceCreateInfo::create(instance_uptr.get(),
                                                                         device_uptr.get(),
                                                                         windowAsync_uptr->GetWindowPtr());

        renderingSurface_uptr = Anvil::RenderingSurface::create(std::move(create_info_ptr));
    }

    renderingSurface_uptr->set_name("Main rendering surface");

    switch (device_uptr->get_type())
    {
    case Anvil::DeviceType::SINGLE_GPU:
        {
            Anvil::SGPUDevice* sgpu_device_ptr(dynamic_cast<Anvil::SGPUDevice*>(device_uptr.get()));

            swapchain_uptr = sgpu_device_ptr->create_swapchain(renderingSurface_uptr.get(),
                                                               windowAsync_uptr->GetWindowPtr(),
                                                               swapchain_format,
                                                               Anvil::ColorSpaceKHR::SRGB_NONLINEAR_KHR,
                                                               swapchain_present_mode,
                                                               swapchain_usage,
                                                               swapchainImagesCount);

            break;
        }

    default:
        {
            anvil_assert(false);
        }
    }
}

void VulkanInit::OnValidationCallback(Anvil::DebugMessageSeverityFlags in_severity,
                                       const char* in_message_ptr)
{
    if ((in_severity & Anvil::DebugMessageSeverityFlagBits::ERROR_BIT) != 0)
    {
        printf("[!] %s\n",
               in_message_ptr);
    }
}
