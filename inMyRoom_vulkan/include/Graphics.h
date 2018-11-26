#pragma once

#include <memory>

#include "configuru.hpp"
#include "glm/mat4x4.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "WindowWithAsyncInput.h"
#include "MovementBaseClass.h"

class Graphics
{
public:
	Graphics(configuru::Config& in_cfgFile);
	~Graphics();

	void init				();
	void register_window_callback  (Anvil::CallbackID in_callback_id, Anvil::CallbackFunction in_callback_function, void* in_callback_owner_ptr);

	void bind_camera		(MovementBaseClass* in_camera);

	void draw_frame			();


private:
	VkBool32 on_validation_callback(VkDebugReportFlagsEXT      message_flags,
									VkDebugReportObjectTypeEXT object_type,
									const char*                layer_prefix,
									const char*                message);

	void deinit		();

	void init_vulkan();
	void init_window_with_async_input_ptr();
	void init_swapchain();

	void init_buffers();
	void init_dsgs();
	void init_images();
	void init_semaphores();
	void init_shaders();

	void init_framebuffers();
	void init_renderpasses();
	void init_pipelines();
	void init_command_buffers();

private:
	configuru::Config& cfgFile;
	MovementBaseClass* camera;

	const float						 m_fov_deg;

	const unsigned int				 m_n_swapchain_images;

	Anvil::Queue*                    m_present_queue_ptr;

	Anvil::BaseDeviceUniquePtr		 m_device_ptr;
	Anvil::InstanceUniquePtr		 m_instance_ptr;

	WindowWithAsyncInputUniquePtr	 window_with_async_input_ptr;

	Anvil::SwapchainUniquePtr		 m_swapchain_ptr;
	Anvil::RenderingSurfaceUniquePtr m_rendering_surface_ptr;

	Anvil::BufferUniquePtr			m_index_buffer_ptr;
	Anvil::BufferUniquePtr			m_vertex_buffer_ptr;
	Anvil::BufferUniquePtr			m_camera_buffer_ptr;
	Anvil::BufferUniquePtr			m_perspective_buffer_ptr;

	Anvil::DescriptorSetGroupUniquePtr	m_dsg_ptr;

	Anvil::ImageUniquePtr           m_depth_image_ptr;
	Anvil::ImageViewUniquePtr       m_depth_image_view_ptr;

	std::vector<Anvil::SemaphoreUniquePtr> m_frame_signal_semaphores;
	std::vector<Anvil::SemaphoreUniquePtr> m_frame_wait_semaphores;

	std::unique_ptr<Anvil::ShaderModuleStageEntryPoint> m_fs_ptr;
	std::unique_ptr<Anvil::ShaderModuleStageEntryPoint> m_vs_ptr;

	std::vector<Anvil::FramebufferUniquePtr>        m_framebuffers;

	Anvil::RenderPassUniquePtr						m_renderpass_ptr;

	Anvil::PipelineID						m_pipeline_id;

	std::vector<Anvil::PrimaryCommandBufferUniquePtr> m_cmd_buffers;
	uint32_t						 m_n_last_semaphore_used;

	Anvil::SubPassID              m_subpass_id;

	uint32_t               m_n_indices;
};
