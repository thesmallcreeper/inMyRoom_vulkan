#pragma once
#include <memory>
#include <string>

#include "configuru.hpp"

#include "misc/window_factory.h"

#include "GameImporter.h"
#include "Graphics.h"
#include "ECS/ECSwrapper.h"
#include "ExportedFunctionsConstructor.h"
#include "InputManager.h"
#include "VulkanInit.h"

class Engine: public VulkanInit
{
public:
    Engine(configuru::Config& in_cfgFile);
    ~Engine();

    Graphics*       GetGraphicsPtr();
    GameImporter*   GetGameImporter();
    ECSwrapper*     GetECSwrapperPtr();

    void Run();


private: // data

    configuru::Config& cfgFile;

    void CallbackFunction_on_close_event(Anvil::CallbackArgument*       in_callback_data_raw_ptr);
    void CallbackFunction_on_keypress_was_up(Anvil::CallbackArgument*   in_callback_data_raw_ptr);
    void CallbackFunction_on_keypress_released(Anvil::CallbackArgument* in_callback_data_raw_ptr);
    void CallbackFunction_on_mouse_movement(Anvil::CallbackArgument*    in_callback_data_raw_ptr);

    std::unique_ptr<GameImporter> gameImporter_uptr;
    std::unique_ptr<Graphics> graphics_uptr;
    std::unique_ptr<ECSwrapper> ECSwrapper_uptr;
    std::unique_ptr<ExportedFunctionsConstructor> exportedFunctionsConstructor_uptr;

    std::unique_ptr<InputManager> inputManager_uptr;

    volatile bool breakMainLoop;
};
