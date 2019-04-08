#pragma once
#include <memory>

#include "configuru.hpp"

#include "misc/window_factory.h"

#include "Graphics.h"
#include "InputManager.h"
#include "CameraBaseClass.h"
#include "VulkanInit.h"

class Engine: public VulkanInit
{
public:
    Engine(configuru::Config& in_cfgFile);
    ~Engine();

    void Run();

private:
    configuru::Config& cfgFile;

    void CallbackFunction_on_close_event(Anvil::CallbackArgument*       in_callback_data_raw_ptr);
    void CallbackFunction_on_keypress_was_up(Anvil::CallbackArgument*   in_callback_data_raw_ptr);
    void CallbackFunction_on_keypress_released(Anvil::CallbackArgument* in_callback_data_raw_ptr);
    void CallbackFunction_on_mouse_movement(Anvil::CallbackArgument*    in_callback_data_raw_ptr);

    std::unique_ptr<Graphics> graphics_uptr;
    std::unique_ptr<CameraBaseClass> camera_uptr;

    InputManager inputManager;

    volatile bool breakMainLoop;
};
