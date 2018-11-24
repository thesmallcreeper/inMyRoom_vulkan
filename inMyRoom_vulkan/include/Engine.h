#pragma once
#include <memory>

#include "configuru.hpp"

#include "misc/window_factory.h"

#include "Graphics.h"
#include "InputManager.h"
#include "MovementBaseClass.h"

class Engine
{
public:
	Engine(configuru::Config& in_cfgFile);
	~Engine();

	void init();
	void run();

private:
	configuru::Config& cfgFile;

	void deinit();

	void callbackFunction_on_close_event(Anvil::CallbackArgument*			in_callback_data_raw_ptr);
	void callbackFunction_on_keypress_was_up(Anvil::CallbackArgument*   in_callback_data_raw_ptr);
	void callbackFunction_on_keypress_released(Anvil::CallbackArgument*		in_callback_data_raw_ptr);
	void callbackFunction_on_mouse_movement(Anvil::CallbackArgument*   in_callback_data_raw_ptr);


	std::unique_ptr<Graphics> graphics_ptr;
	std::unique_ptr<MovementBaseClass> camera_ptr;

	InputManager inputManager;

	volatile bool breakMainLoop;
};