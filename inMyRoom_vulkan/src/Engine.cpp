#include "Engine.h"

#include "Graphics.h"
#include "InputManager.h"

#include "NaiveCamera.h"

Engine::Engine(configuru::Config& in_cfgFile)
	:cfgFile(in_cfgFile),
	 inputManager(cfgFile),
	 breakMainLoop(false)
{
	
}

Engine::~Engine()
{
	deinit();
}

void Engine::init()
{
	inputManager.init();

	graphics_ptr = std::make_unique<Graphics>(cfgFile);
	graphics_ptr->init();
	graphics_ptr->register_window_callback(Anvil::WINDOW_CALLBACK_ID_ABOUT_TO_CLOSE,
									       std::bind(&Engine::callbackFunction_on_close_event,
												     this,
												     std::placeholders::_1),
										   this );
	graphics_ptr->register_window_callback(Anvil::WINDOW_CALLBACK_ID_KEYPRESS_PRESSED_WAS_UP,
									       std::bind(&Engine::callbackFunction_on_keypress_was_up,
												     this,
												     std::placeholders::_1),
									       this );
	graphics_ptr->register_window_callback(Anvil::WINDOW_CALLBACK_ID_KEYPRESS_RELEASED,
									       std::bind(&Engine::callbackFunction_on_keypress_released,
												     this,
												     std::placeholders::_1),
									       this );
	graphics_ptr->register_window_callback(Anvil::WINDOW_CALLBACK_ID_MOUSE_MOVED,
									       std::bind(&Engine::callbackFunction_on_mouse_movement,
												     this,
												     std::placeholders::_1),
									       this );

	camera_ptr = std::make_unique<NaiveCamera>(cfgFile["NaiveCamera"]["Speed"].as_float());

	inputManager.bindCameraFreezeOldUnfreezeNew(camera_ptr.get());
}

void Engine::deinit()
{
	graphics_ptr.reset();
}

void Engine::callbackFunction_on_close_event(Anvil::CallbackArgument*   in_callback_data_raw_ptr)
{
	breakMainLoop = true;
}

void Engine::callbackFunction_on_keypress_was_up(Anvil::CallbackArgument*   in_callback_data_raw_ptr)
{
	auto callback_data_ptr = static_cast<Anvil::OnKeypressPressedWasUpCallbackArgument*>(in_callback_data_raw_ptr);

	inputManager.keyPressed(callback_data_ptr->pressed_key_id);
}

void Engine::callbackFunction_on_keypress_released(Anvil::CallbackArgument*   in_callback_data_raw_ptr)
{
	auto callback_data_ptr = static_cast<Anvil::OnKeypressReleasedCallbackArgument*>(in_callback_data_raw_ptr);

	inputManager.keyReleased(callback_data_ptr->released_key_id);
}

void Engine::callbackFunction_on_mouse_movement(Anvil::CallbackArgument*   in_callback_data_raw_ptr)
{
	auto callback_data_ptr = static_cast<Anvil::OnMouseMovementCallbackArgument*>(in_callback_data_raw_ptr);

	inputManager.mouseMoved(callback_data_ptr->xOffset, callback_data_ptr->yOffset);
}

void Engine::run()
{
	while (!breakMainLoop) 
	{
		for (auto eventIterator : inputManager.grabAndResetEventVector())
		{
			if (eventIterator == SHOULD_CLOSE)
				breakMainLoop = true;
		}
	}
}