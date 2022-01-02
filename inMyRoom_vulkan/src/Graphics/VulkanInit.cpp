#include "Graphics/VulkanInit.h"

#include <iostream>
#include <algorithm>

#ifdef _DEBUG
#define ENABLE_VALIDATION
#endif

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

VulkanInit::VulkanInit(const configuru::Config& in_cfgFile,
                       std::string appName)
    :
    cfgFile(in_cfgFile),
    windowWidth(in_cfgFile["graphicsSettings"]["xRes"].as_integer<unsigned int>()),
    windowHeight(in_cfgFile["graphicsSettings"]["yRes"].as_integer<unsigned int>())
{

    //
    // Initialize instance!
    std::vector<std::string> vulkan_layers;
#ifdef ENABLE_VALIDATION
    vulkan_layers.emplace_back("VK_LAYER_KHRONOS_validation");
    vulkan_layers.emplace_back("VK_LAYER_KHRONOS_synchronization2");
#endif

    std::vector<std::string> vulkan_instance_extensions = WindowWithAsyncInput::GetRequiredInstanceExtensions();
    bool enableDebugMessenger = false;
#ifdef _DEBUG
    vulkan_instance_extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    enableDebugMessenger = true;
#endif

    CreateInstance(appName,
                   "inMyRoom_vulkan",
                   vulkan_layers,
                   vulkan_instance_extensions,
                   VK_API_VERSION_1_2,
                   enableDebugMessenger);


    //
    // Select device!
    auto physical_devices = vulkanInstance.enumeratePhysicalDevices().value;
    if (physical_devices.empty()) {
        std::cerr << "No physical device found!\n";
        std::terminate();
    }
    vk::PhysicalDevice selected_device = physical_devices.front();

    std::string device_preferred_name = cfgFile["graphicsSettings"]["gpuPreferred"].as_string();
    if(device_preferred_name != "") {
        auto result = std::find_if(physical_devices.cbegin(), physical_devices.cend(),
                               [device_preferred_name](const vk::PhysicalDevice& this_device)
                                   {return std::string(this_device.getProperties().deviceName.data()) == device_preferred_name;});

        if (result != physical_devices.end())
            selected_device = *result;
        else {
            std::cout << "Preferred GPU not found, listing all the GPUs names:\n";
            for(const vk::PhysicalDevice& this_device: physical_devices)
            {
                std::cout << "-\"" << this_device.getProperties().deviceName << "\"\n";
            }
        }
    }

    std::cout << "Device \"" << selected_device.getProperties().deviceName <<  "\" chosen\n";

    //
    // Create device
    std::vector<std::string> vulkan_device_extensions;
    vulkan_device_extensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    vulkan_device_extensions.emplace_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
    vulkan_device_extensions.emplace_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
    vulkan_device_extensions.emplace_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
    vulkan_device_extensions.emplace_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);

    vk::PhysicalDeviceFeatures vulkan_device_features;
    vulkan_device_features.samplerAnisotropy = VK_TRUE;
    vk::PhysicalDeviceVulkan11Features vulkan11_device_features;
    vulkan11_device_features.storageBuffer16BitAccess = VK_TRUE;
    vk::PhysicalDeviceVulkan12Features vulkan12_device_features;
    vulkan12_device_features.descriptorIndexing = VK_TRUE;
    vulkan12_device_features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    vulkan12_device_features.runtimeDescriptorArray = VK_TRUE;
    vulkan12_device_features.descriptorBindingVariableDescriptorCount = VK_TRUE;
    vulkan12_device_features.descriptorBindingPartiallyBound = VK_TRUE;
    vulkan12_device_features.timelineSemaphore = VK_TRUE;

    void* pNext = &vulkan11_device_features;
    vulkan11_device_features.pNext = &vulkan12_device_features;
    CreateDevice(selected_device,
                 1, 0, 0,
                 vulkan_device_extensions,
                 vulkan_device_features,
                 pNext);

    //
    // Initialize VMA allocator
    vma::AllocatorCreateFlags allocator_flags = vma::AllocatorCreateFlagBits::eKhrDedicatedAllocation;
    InitializeVMA(allocator_flags, VK_API_VERSION_1_2);

    //
    // Create window!
    CreateWindow_(appName, windowWidth, windowHeight);

    //
    // Select present format !
    auto supported_formats = physicalDevice.getSurfaceFormatsKHR(surface).value;
    vk::SurfaceFormatKHR chosen_surface_format;
    for (const vk::SurfaceFormatKHR& this_surface_format : preferredSurfaceFormatOrder) {
        auto result = std::find(supported_formats.begin(), supported_formats.end(), this_surface_format);
        if (result != supported_formats.end()) {
            chosen_surface_format = *result;
            break;
        }
    }

    // Select present mode!
    vk::PresentModeKHR requested_mode;
    {
        auto result = presentAliasToPresentMode_map.find(cfgFile["graphicsSettings"]["presentMode"].as_string());
        if (result != presentAliasToPresentMode_map.end()) {
            requested_mode = result->second;
        } else {
            std::cout << "Requested present mode not valid! Fallback to FIFO.\n";
            requested_mode = vk::PresentModeKHR::eFifo;
        }
    }

    auto supported_present_modes = physicalDevice.getSurfacePresentModesKHR(surface).value;
    vk::PresentModeKHR chosen_present_mode;
    {
        auto result = std::find(supported_present_modes.begin(), supported_present_modes.end(), requested_mode);
        if (result != supported_present_modes.end()) {
            chosen_present_mode = *result;
        } else {
            std::cout << "Requested present mode not supported! Fallback to FIFO.\n";
            chosen_present_mode = vk::PresentModeKHR::eFifo;
        }
    }

    // Select swapchain images count
    auto surface_capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface).value;
    uint32_t swapchain_images_count = std::max(uint32_t(3), surface_capabilities.minImageCount);

    //
    // Create swapchain!
    CreateSwapchain(chosen_surface_format,
                    chosen_present_mode,
                    swapchain_images_count,
                    vk::ImageUsageFlagBits::eColorAttachment);

}

VulkanInit::~VulkanInit()
{
    for (const auto& this_imageView : swapchainImageViews) {
        device.destroy(this_imageView);
    }
    device.destroy(swapchain);
    windowAsync_uptr.reset();
    vma_allocator.destroy();
    device.destroy();
    vulkanInstance.destroy(surface);
    if(vulkanDebugUtilsMessenger)
        vulkanInstance.destroyDebugUtilsMessengerEXT( vulkanDebugUtilsMessenger );
    vulkanInstance.destroy();
}

void VulkanInit::CreateInstance(const std::string&              appName,
                                const std::string&              engineName,
                                const std::vector<std::string>& layers,
                                const std::vector<std::string>& extensions,
                                uint32_t                        apiVersion,
                                bool                            enableDebugMessenger)
{
    // Hook the dynamic library!
    static vk::DynamicLoader dl;
    auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>( "vkGetInstanceProcAddr" );
    VULKAN_HPP_DEFAULT_DISPATCHER.init( vkGetInstanceProcAddr );

    // Initialize instance!
    vk::ApplicationInfo applicationInfo( appName.c_str(), 1, engineName.c_str(), 1, apiVersion );

    std::vector<const char*> layers_c_ptrs;
    std::transform(layers.cbegin(), layers.cend(), std::back_inserter(layers_c_ptrs),
                   [](const std::string& str) {return str.c_str();});

    std::vector<const char*> extensions_c_ptrs;
    std::transform(extensions.cbegin(), extensions.cend(), std::back_inserter(extensions_c_ptrs),
                   [](const std::string& str) {return str.c_str();});

    vk::InstanceCreateInfo instanceCreateInfo({},
                                              &applicationInfo,
                                              layers_c_ptrs,
                                              extensions_c_ptrs);

    auto instance_opt = vk::createInstance(instanceCreateInfo);
    if(instance_opt.result == vk::Result::eSuccess) {
        vulkanInstance = instance_opt.value;
    }
    else {
        if (instance_opt.result != vk::Result::eErrorIncompatibleDriver)
            std::cerr << "Cannot create instance! Incompatible driver\n";
        else if (instance_opt.result != vk::Result::eErrorLayerNotPresent)
            std::cerr << "Cannot create instance! Layer not present\n";
        else if (instance_opt.result != vk::Result::eErrorExtensionNotPresent)
            std::cerr << "Cannot create instance! Extension not present\n";
        else
            std::cerr << "Cannot create instance! Other reason\n";

        std::terminate();
    }

    // Link dispatcher!
    VULKAN_HPP_DEFAULT_DISPATCHER.init( vulkanInstance );

    // Enable debug messenger
    if (enableDebugMessenger) {
        vk::DebugUtilsMessengerCreateInfoEXT messengerCreateInfo
            ({},
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
            &debugUtilsMessengerCallback);

        vulkanDebugUtilsMessenger = vulkanInstance.createDebugUtilsMessengerEXT(messengerCreateInfo).value;
    }
}

void VulkanInit::CreateDevice(vk::PhysicalDevice physical_device,
                              uint32_t graphics_count,
                              uint32_t dedicated_compute_count,
                              uint32_t dedicated_transfer_count,
                              const std::vector<std::string>& extensions,
                              const vk::PhysicalDeviceFeatures& features,
                              void* create_device_pNext)
{
    assert(graphics_count != 0);

    // Find queues
    auto queue_families_properties = physical_device.getQueueFamilyProperties();
    auto graphics_family = std::find_if(queue_families_properties.begin(), queue_families_properties.end(),
                                        [](const vk::QueueFamilyProperties& this_family_props)
                                        {return (this_family_props.queueFlags & vk::QueueFlagBits::eGraphics);});

    auto dedi_compute_family = std::find_if(queue_families_properties.begin(), queue_families_properties.end(),
                                            [](const vk::QueueFamilyProperties& this_family_props)
                                            {return !(this_family_props.queueFlags & vk::QueueFlagBits::eGraphics)
                                                && (this_family_props.queueFlags & vk::QueueFlagBits::eCompute);});

    auto dedi_transfer_family = std::find_if(queue_families_properties.begin(), queue_families_properties.end(),
                                            [](const vk::QueueFamilyProperties& this_family_props)
                                            {return !(this_family_props.queueFlags & vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute)
                                                    && (this_family_props.queueFlags & vk::QueueFlagBits::eTransfer);});

    std::vector<vk::DeviceQueueCreateInfo> device_queues_create_infos;
     std::vector<std::unique_ptr<std::vector<float>>> queues_priorities_uptrs;
    if (graphics_family != queue_families_properties.end()) {
        uint32_t queues_count = std::min(graphics_count, graphics_family->queueCount);

        std::unique_ptr<std::vector<float>> this_queues_priorities_uptr = std::make_unique<std::vector<float>>(queues_count, 1.f);
        vk::DeviceQueueCreateInfo graphics_queue_create_info({},
                                                             uint32_t(std::distance(queue_families_properties.begin(), graphics_family)),
                                                             *this_queues_priorities_uptr);

        device_queues_create_infos.emplace_back(graphics_queue_create_info);
        queues_priorities_uptrs.emplace_back(std::move(this_queues_priorities_uptr));
    }
    else {
        std::cerr << "Couldn't find graphics queue.\n";
        std::terminate();
    }

    if (dedicated_compute_count && dedi_compute_family != queue_families_properties.end()) {
        uint32_t queues_count = std::min(dedicated_compute_count, dedi_compute_family->queueCount);

        std::unique_ptr<std::vector<float>> this_queues_priorities_uptr = std::make_unique<std::vector<float>>(queues_count, 1.f);
        vk::DeviceQueueCreateInfo dedi_compute_queue_create_info({},
                                                                 uint32_t(std::distance(queue_families_properties.begin(), dedi_compute_family)),
                                                                 *this_queues_priorities_uptr);

        device_queues_create_infos.emplace_back(dedi_compute_queue_create_info);
        queues_priorities_uptrs.emplace_back(std::move(this_queues_priorities_uptr));
    }

    if (dedicated_transfer_count && dedi_transfer_family != queue_families_properties.end()) {
        uint32_t queues_count = std::min(dedicated_transfer_count, dedi_transfer_family->queueCount);

        std::unique_ptr<std::vector<float>> this_queues_priorities_uptr = std::make_unique<std::vector<float>>(queues_count, 1.f);
        vk::DeviceQueueCreateInfo dedi_transfer_queue_create_info({},
                                                                  uint32_t(std::distance(queue_families_properties.begin(), dedi_transfer_family)),
                                                                  *this_queues_priorities_uptr);

        device_queues_create_infos.emplace_back(dedi_transfer_queue_create_info);
        queues_priorities_uptrs.emplace_back(std::move(this_queues_priorities_uptr));
    }

    // Transform extensions
    std::vector<const char*> extensions_c_ptrs;
    std::transform(extensions.cbegin(), extensions.cend(), std::back_inserter(extensions_c_ptrs),
                   [](const std::string& str) {return str.c_str();});

    // Create device!
    vk::DeviceCreateInfo device_create_info({},
                                            device_queues_create_infos,
                                            {},
                                            extensions_c_ptrs,
                                            &features);

    device_create_info.pNext = create_device_pNext;

    auto device_opt = physical_device.createDevice(device_create_info);
    if (device_opt.result == vk::Result::eSuccess) {
        device = device_opt.value;
        physicalDevice = physical_device;
    }
    else {
        if (device_opt.result == vk::Result::eErrorExtensionNotPresent
            || device_opt.result == vk::Result::eErrorFeatureNotPresent)
        {
            std::cerr << "Cannot create device! Extension or feature not present.\n";
        }
        else {
            std::cerr << "Cannot create device! Other reason.\n";
        }

        std::terminate();
    }

    // Link dispatcher!
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);

    // Get the queues
    for(const vk::DeviceQueueCreateInfo& this_queue_create_info: device_queues_create_infos) {
        for(uint32_t index = 0; index != this_queue_create_info.queueCount; ++index) {
            vk::Queue this_queue = device.getQueue(this_queue_create_info.queueFamilyIndex, index);
            if (queue_families_properties[this_queue_create_info.queueFamilyIndex].queueFlags & vk::QueueFlagBits::eGraphics)
                queues.graphicsQueues.emplace_back(this_queue, this_queue_create_info.queueFamilyIndex);
            else if (queue_families_properties[this_queue_create_info.queueFamilyIndex].queueFlags & vk::QueueFlagBits::eCompute)
                queues.dedicatedComputeQueues.emplace_back(this_queue, this_queue_create_info.queueFamilyIndex);
            else if (queue_families_properties[this_queue_create_info.queueFamilyIndex].queueFlags & vk::QueueFlagBits::eTransfer)
                queues.dedicatedTransferQueues.emplace_back(this_queue, this_queue_create_info.queueFamilyIndex);
        }
    }

    assert(queues.graphicsQueues.size());
}


void VulkanInit::InitializeVMA(vma::AllocatorCreateFlags allocator_flags,
                               uint32_t                  apiVersion)
{
    vma_vulkanFunctions_uptr = std::make_unique<vma::VulkanFunctions>();
    vma_vulkanFunctions_uptr->vkGetInstanceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr;
    vma_vulkanFunctions_uptr->vkGetDeviceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr;

    vma::AllocatorCreateInfo allocator_create_info;
    allocator_create_info.vulkanApiVersion = apiVersion;
    allocator_create_info.physicalDevice = physicalDevice;
    allocator_create_info.device = device;
    allocator_create_info.instance = vulkanInstance;
    allocator_create_info.pVulkanFunctions = vma_vulkanFunctions_uptr.get();

    auto allocator_opt = vma::createAllocator(allocator_create_info);
    if (allocator_opt.result == vk::Result::eSuccess) {
        vma_allocator = allocator_opt.value;
    } else {
        std::cerr << "Failed to create VMA allocator\n";
        std::terminate();
    }
}


void VulkanInit::CreateWindow_(const std::string& title,
                               uint32_t width,
                               uint32_t height)
{
    windowAsync_uptr = std::make_unique<WindowWithAsyncInput>(title, width, height);

    VkSurfaceKHR vk_surface;
    VkResult err = glfwCreateWindowSurface( static_cast<VkInstance>( vulkanInstance ), windowAsync_uptr->GetGlfwWindow(), nullptr, &vk_surface );
    assert(err == VK_SUCCESS);

    if ( err != VK_SUCCESS ) {
        std::cerr << "Cannot create window surface!\n";
        std::terminate();
    }
    surface = vk::SurfaceKHR( vk_surface );
}

void VulkanInit::CreateSwapchain(vk::SurfaceFormatKHR surface_format,
                                 vk::PresentModeKHR present_mode,
                                 uint32_t swapchain_count,
                                 vk::ImageUsageFlags usage)
{
    auto surface_capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface).value;
    VkExtent2D                 swapchain_extent;
    if ( surface_capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max() ) {
        swapchain_extent.width = std::clamp( windowWidth, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width );
        swapchain_extent.height = std::clamp( windowHeight, surface_capabilities.minImageExtent.height, surface_capabilities.minImageExtent.height );
    }
    else {
        swapchain_extent.height = surface_capabilities.currentExtent.height;
        swapchain_extent.width = surface_capabilities.currentExtent.width;
    }

    vk::SwapchainCreateInfoKHR swapchain_create_info({},
                                                     surface,
                                                     swapchain_count,
                                                     surface_format.format,
                                                     surface_format.colorSpace,
                                                     swapchain_extent,
                                                     1,
                                                     usage,
                                                     vk::SharingMode::eExclusive,
                                                     {},
                                                     vk::SurfaceTransformFlagBitsKHR::eIdentity,
                                                     vk::CompositeAlphaFlagBitsKHR::eOpaque,
                                                     present_mode,
                                                     true,
                                                     {});

    auto swapchair_opt = device.createSwapchainKHR(swapchain_create_info);
    if (swapchair_opt.result == vk::Result::eSuccess) {
        swapchain = swapchair_opt.value;
        swapchainCreateInfo = swapchain_create_info;
    } else {
        std::cerr << "Failed to create swapchain.\n";
        std::terminate();
    }

    auto images = device.getSwapchainImagesKHR(swapchain).value;

    for (vk::Image this_image: images) {
        vk::ImageViewCreateInfo imageview_create_info({},
                                                      this_image,
                                                      vk::ImageViewType::e2D,
                                                      surface_format.format,
                                                      {},
                                                      { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

        swapchainImageViews.emplace_back(device.createImageView(imageview_create_info).value);
    }
}

// TODO: Double check compatibility of licenses
// The function "debugUtilsMessengerCallback" comes from a file with the following license:
// Copyright(c) 2019, NVIDIA CORPORATION. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
VKAPI_ATTR VkBool32 VKAPI_CALL
debugUtilsMessengerCallback( VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
                             VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
                             VkDebugUtilsMessengerCallbackDataEXT const * pCallbackData,
                             void * /*pUserData*/ )
{
#if !defined( NDEBUG )
    if ( pCallbackData->messageIdNumber == 648835635 )
    {
        // UNASSIGNED-khronos-Validation-debug-build-warning-message
        return VK_FALSE;
    }
    if ( pCallbackData->messageIdNumber == 767975156 )
    {
        // UNASSIGNED-BestPractices-vkCreateInstance-specialuse-extension
        return VK_FALSE;
    }
#endif

    std::cerr << vk::to_string( static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>( messageSeverity ) ) << ": "
              << vk::to_string( static_cast<vk::DebugUtilsMessageTypeFlagsEXT>( messageTypes ) ) << ":\n";
    std::cerr << "\t"
              << "messageIDName   = <" << pCallbackData->pMessageIdName << ">\n";
    std::cerr << "\t"
              << "messageIdNumber = " << pCallbackData->messageIdNumber << "\n";
    std::cerr << "\t"
              << "message         = <" << pCallbackData->pMessage << ">\n";
    if ( 0 < pCallbackData->queueLabelCount )
    {
        std::cerr << "\t"
                  << "Queue Labels:\n";
        for ( uint8_t i = 0; i < pCallbackData->queueLabelCount; i++ )
        {
            std::cerr << "\t\t"
                      << "labelName = <" << pCallbackData->pQueueLabels[i].pLabelName << ">\n";
        }
    }
    if ( 0 < pCallbackData->cmdBufLabelCount )
    {
        std::cerr << "\t"
                  << "CommandBuffer Labels:\n";
        for ( uint8_t i = 0; i < pCallbackData->cmdBufLabelCount; i++ )
        {
            std::cerr << "\t\t"
                      << "labelName = <" << pCallbackData->pCmdBufLabels[i].pLabelName << ">\n";
        }
    }
    if ( 0 < pCallbackData->objectCount )
    {
        std::cerr << "\t"
                  << "Objects:\n";
        for ( uint8_t i = 0; i < pCallbackData->objectCount; i++ )
        {
            std::cerr << "\t\t"
                      << "Object " << i << "\n";
            std::cerr << "\t\t\t"
                      << "objectType   = "
                      << vk::to_string( static_cast<vk::ObjectType>( pCallbackData->pObjects[i].objectType ) ) << "\n";
            std::cerr << "\t\t\t"
                      << "objectHandle = " << pCallbackData->pObjects[i].objectHandle << "\n";
            if ( pCallbackData->pObjects[i].pObjectName )
            {
                std::cerr << "\t\t\t"
                          << "objectName   = <" << pCallbackData->pObjects[i].pObjectName << ">\n";
            }
        }
    }
    return VK_TRUE;
}

