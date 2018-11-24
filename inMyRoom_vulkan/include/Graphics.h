#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <memory>

#include "configuru.hpp"

#include "WindowWithAsyncInput.h"

class Graphics
{
public:
	Graphics(configuru::Config& in_cfgFile);
	~Graphics();

	void init				();
	void register_window_callback  (Anvil::CallbackID in_callback_id, Anvil::CallbackFunction in_callback_function, void* in_callback_owner_ptr);

	void draw_frame			();


private:
	configuru::Config& cfgFile;

	void deinit		();

	void init_vulkan();
	void init_window_with_async_input_ptr();
	void init_swapchain();

	const unsigned int				 m_n_swapchain_images;

	Anvil::Queue*                    m_present_queue_ptr;

	Anvil::BaseDeviceUniquePtr		 m_device_ptr;
	Anvil::InstanceUniquePtr		 m_instance_ptr;

	WindowWithAsyncInputUniquePtr	 window_with_async_input_ptr;

	Anvil::SwapchainUniquePtr		 m_swapchain_ptr;
	Anvil::RenderingSurfaceUniquePtr m_rendering_surface_ptr;





	VkBool32 on_validation_callback(VkDebugReportFlagsEXT      message_flags,
									VkDebugReportObjectTypeEXT object_type,
									const char*                layer_prefix,
									const char*                message);
};
