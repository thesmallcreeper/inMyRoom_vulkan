#include "Graphics.h"

#include "config.h"

#include "misc/window_factory.h"
#include "misc/swapchain_create_info.h"
#include "wrappers/device.h"
#include "wrappers/event.h"
#include "wrappers/instance.h"
#include "wrappers/image.h"
#include "wrappers/image_view.h"
#include "wrappers/rendering_surface.h"

#include <thread>
#include <future>

#include "WindowWithAsyncInput.h"


Graphics::Graphics(configuru::Config& in_cfgFile)
	:cfgFile(in_cfgFile),
	 m_n_swapchain_images(cfgFile["graphicsSettings"]["swapchain_images"].as_integer<unsigned int>())
{

}

Graphics::~Graphics()
{
	deinit();
}

void Graphics::init()
{
	init_vulkan();
	init_window_with_async_input_ptr();
	init_swapchain();
	
}

void Graphics::deinit()
{
	Anvil::Vulkan::vkDeviceWaitIdle(m_device_ptr->get_device_vk());

	m_rendering_surface_ptr.reset();
	m_swapchain_ptr.reset();
	
	m_device_ptr.reset();
	m_instance_ptr.reset();
	
	window_with_async_input_ptr.reset();
}

void Graphics::init_vulkan()
{
	/* Create a Vulkan instance */
	m_instance_ptr = Anvil::Instance::create("inMyRoom_vulkan",  /* app_name */
											 "inMyRoom_vulkan",  /* engine_name */
#ifdef ENABLE_VALIDATION
											 std::bind(&App::on_validation_callback,
												 this,
												 std::placeholders::_1,
												 std::placeholders::_2,
												 std::placeholders::_3,
												 std::placeholders::_4),
#else
											 Anvil::DebugCallbackFunction(),
#endif
											 false);			 /* in_mt_safe */

	/* Determine which extensions we need to request for */
	{
		/* Create a Vulkan device */
		m_device_ptr = Anvil::SGPUDevice::create(m_instance_ptr->get_physical_device(0),
												 true, /* in_enable_shader_module_cache */
												 Anvil::DeviceExtensionConfiguration(),
												 std::vector<std::string>(), /* in_layers                               */
												 false,                      /* in_transient_command_buffer_allocs_only */
												 false);                     /* in_support_resettable_command_buffers   */
	}
}

void Graphics::init_window_with_async_input_ptr()
{
    #ifdef _WIN32
        const Anvil::WindowPlatform platform = Anvil::WINDOW_PLATFORM_SYSTEM;
    #else
        const Anvil::WindowPlatform platform = Anvil::WINDOW_PLATFORM_XCB;
    #endif
	
	window_with_async_input_ptr = std::make_unique<WindowWithAsyncInput>(platform,
																		 "inMyRoom_vulkan",
																		 cfgFile["graphicsSettings"]["xRes"].as_integer<unsigned int>(),
																		 cfgFile["graphicsSettings"]["yRes"].as_integer<unsigned int>(),
																		 true);
}

void Graphics::init_swapchain()
{
	static const Anvil::Format          swapchain_format(Anvil::Format::B8G8R8A8_UNORM);
	static const Anvil::PresentModeKHR  swapchain_present_mode(Anvil::PresentModeKHR::FIFO_KHR);
	static const Anvil::ImageUsageFlags swapchain_usage(Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT | Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT);

	m_rendering_surface_ptr = Anvil::RenderingSurface::create(m_instance_ptr.get(),
															  m_device_ptr.get(),
															  window_with_async_input_ptr -> m_window_ptr.get());

	m_rendering_surface_ptr->set_name("Main rendering surface");


	switch (m_device_ptr->get_type())
	{
	case Anvil::DeviceType::SINGLE_GPU:
	{
		Anvil::SGPUDevice* sgpu_device_ptr(dynamic_cast<Anvil::SGPUDevice*>(m_device_ptr.get()));

		m_swapchain_ptr = sgpu_device_ptr->create_swapchain(m_rendering_surface_ptr.get(),
			window_with_async_input_ptr -> m_window_ptr.get(),
			swapchain_format,
			Anvil::ColorSpaceKHR::SRGB_NONLINEAR_KHR,
			swapchain_present_mode,
			swapchain_usage,
			m_n_swapchain_images);

		/* Cache the queue we are going to use for presentation */
		const std::vector<uint32_t>* present_queue_fams_ptr = nullptr;

		if (!m_rendering_surface_ptr->get_queue_families_with_present_support(sgpu_device_ptr->get_physical_device(),
			&present_queue_fams_ptr))
		{
			anvil_assert_fail();
		}

		m_present_queue_ptr = sgpu_device_ptr->get_queue_for_queue_family_index(present_queue_fams_ptr->at(0),
			0); /* in_n_queue */

		break;
	}

	default:
	{
		anvil_assert(false);
	}
	}
}


void Graphics::register_window_callback(Anvil::CallbackID in_callback_id, Anvil::CallbackFunction in_callback_function, void* in_callback_owner_ptr)
{
	window_with_async_input_ptr->m_window_ptr->register_for_callbacks(in_callback_id,
																	  in_callback_function,
																	  in_callback_owner_ptr);
}


VkBool32 Graphics::on_validation_callback(VkDebugReportFlagsEXT      message_flags,
										  VkDebugReportObjectTypeEXT object_type,
										  const char*                layer_prefix,
										  const char*                message)
{
	if ((message_flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) != 0)
	{
		fprintf(stderr,
			"[!] %s\n",
			message);
	}

	return false;
}
