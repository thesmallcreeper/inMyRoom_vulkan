#pragma once

#include <memory>
#include <thread>
#include <future>
#include <fstream>

#include "configuru.hpp"
#include "glm/mat4x4.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "tiny_gltf.h"

#include "WindowWithAsyncInput.h"
#include "MovementBaseClass.h"

#include "PrimitivesPipelines.h"
#include "SceneMeshes.h"
#include "NodesTree.h"

class Graphics
{
public:
	Graphics(configuru::Config& in_cfgFile);
	~Graphics();

	void init				();

	void unregister_window_callback(Anvil::CallbackID in_callback_id, Anvil::CallbackFunction in_callback_function, void* in_callback_owner_ptr);
	void register_window_callback  (Anvil::CallbackID in_callback_id, Anvil::CallbackFunction in_callback_function, void* in_callback_owner_ptr);

	void bind_camera		(MovementBaseClass* in_camera);

	void draw_frame			();


private:
	void on_validation_callback(Anvil::DebugMessageSeverityFlags in_severity,
								const char*                      in_message_ptr);

	std::string GetFilePathExtension(const std::string &FileName);

	void deinit		();

	void load_scene();

	void init_vulkan();
	void init_window_with_async_input_ptr();
	void init_swapchain();

	void init_camera_buffers();
	void init_scene_nodes();
	void init_dsgs();
	void init_images();
	void init_semaphores();
	void init_shaders();

	void init_framebuffers();
	void init_renderpasses();
	void init_scene_meshes();
	void init_command_buffers();

private:
	configuru::Config& cfgFile;
	tinygltf::Model model;

	std::unique_ptr<PrimitivesPipelines> pipelinesOfPrimitives_ptr;
	std::unique_ptr<SceneMeshes> meshesOfScene_ptr;
	std::unique_ptr<NodesTree> treeOfNodes_ptr;

	MovementBaseClass* camera;

	const float						 m_fov_deg;

	const unsigned int				 m_n_swapchain_images;

	Anvil::Queue*                    m_present_queue_ptr;

	Anvil::BaseDeviceUniquePtr		 m_device_ptr;
	Anvil::InstanceUniquePtr		 m_instance_ptr;

	WindowWithAsyncInputUniquePtr	 window_with_async_input_ptr;

	Anvil::SwapchainUniquePtr		 m_swapchain_ptr;
	Anvil::RenderingSurfaceUniquePtr m_rendering_surface_ptr;

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

	std::vector<Anvil::PrimaryCommandBufferUniquePtr> m_cmd_buffers;
	uint32_t						 m_n_last_semaphore_used;

	Anvil::SubPassID              m_subpass_id;
};
