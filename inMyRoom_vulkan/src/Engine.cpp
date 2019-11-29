#include "Engine.h"

#include "InputManager.h"
#include "configuru.hpp"

#include "NaiveCamera.h"

#include "ECS/GeneralComponents/PositionComp.h"
#include "ECS/GeneralComponents/NodeGlobalMatrixComp.h"

Engine::Engine(configuru::Config& in_cfgFile)
    :
    VulkanInit(in_cfgFile),
    cfgFile(in_cfgFile),
    inputManager(in_cfgFile),
    breakMainLoop(false)
{
    {   // Enabling mouse/keyboard input
        windowAsync_uptr->GetWindowPtr()->register_for_callbacks(Anvil::WINDOW_CALLBACK_ID_CLOSE_EVENT,
                                                                 std::bind(&Engine::CallbackFunction_on_close_event,
                                                                           this,
                                                                           std::placeholders::_1),
                                                                 this);
        windowAsync_uptr->GetWindowPtr()->register_for_callbacks(Anvil::WINDOW_CALLBACK_ID_KEYPRESS_PRESSED_WAS_UP,
                                                                 std::bind(&Engine::CallbackFunction_on_keypress_was_up,
                                                                           this,
                                                                           std::placeholders::_1),
                                                                 this);
        windowAsync_uptr->GetWindowPtr()->register_for_callbacks(Anvil::WINDOW_CALLBACK_ID_KEYPRESS_RELEASED,
                                                                 std::bind(&Engine::CallbackFunction_on_keypress_released,
                                                                           this,
                                                                           std::placeholders::_1),
                                                                 this);
        windowAsync_uptr->GetWindowPtr()->register_for_callbacks(Anvil::WINDOW_CALLBACK_ID_MOUSE_MOVED,
                                                                 std::bind(&Engine::CallbackFunction_on_mouse_movement,
                                                                           this,
                                                                           std::placeholders::_1),
                                                                 this);
    }

    {   // Initializing engine exported functions
        engineExportedFunctions.engine_ptr = this;
    }

    {   // Initializing ECS
        ECSwrapper_uptr = std::make_unique<ECSwrapper>(&engineExportedFunctions);

        std::unique_ptr<PositionComp> position_comp_uptr= std::make_unique<PositionComp>(ECSwrapper_uptr.get());
        ECSwrapper_uptr->AddComponentAndOwnership(std::move(position_comp_uptr));

        std::unique_ptr<NodeGlobalMatrixComp> nodeGlobalMatrix_comp_uptr = std::make_unique<NodeGlobalMatrixComp>(ECSwrapper_uptr.get());
        ECSwrapper_uptr->AddComponentAndOwnership(std::move(nodeGlobalMatrix_comp_uptr));
    }

    {   // Initializing graphics which add some specific components to ECS
        graphics_uptr = std::make_unique<Graphics>(this, cfgFile, device_uptr.get(), swapchain_uptr.get(),
                                                   windowWidth, windowHeight, swapchainImagesCount);
    }

    {   // Game importing
        std::string game_file_path = cfgFile["game"]["path"].as_string();
        gameImport_uptr = std::make_unique<GameImport>(this, game_file_path);
    }

    {   // camera
        camera_uptr = std::make_unique<NaiveCamera>(cfgFile["NaiveCamera"]["Speed"].as_float(),
                                                    glm::vec3(0.f, 0.f, +1.f), glm::vec3(0.f, 0.f, 0.f));

        graphics_uptr->BindCamera(camera_uptr.get());

        inputManager.BindPlayerInputFreezeOldUnfreezeNew(camera_uptr.get());
    }
}

Engine::~Engine()
{
    windowAsync_uptr->GetWindowPtr()->unregister_from_callbacks(Anvil::WINDOW_CALLBACK_ID_CLOSE_EVENT,
                                                                std::bind(&Engine::CallbackFunction_on_close_event,
                                                                          this,
                                                                          std::placeholders::_1),
                                                                this);
    windowAsync_uptr->GetWindowPtr()->unregister_from_callbacks(Anvil::WINDOW_CALLBACK_ID_KEYPRESS_PRESSED_WAS_UP,
                                                                std::bind(&Engine::CallbackFunction_on_keypress_was_up,
                                                                          this,
                                                                          std::placeholders::_1),
                                                                this);
    windowAsync_uptr->GetWindowPtr()->unregister_from_callbacks(Anvil::WINDOW_CALLBACK_ID_KEYPRESS_RELEASED,
                                                                std::bind(&Engine::CallbackFunction_on_keypress_released,
                                                                          this,
                                                                          std::placeholders::_1),
                                                                this);
    windowAsync_uptr->GetWindowPtr()->unregister_from_callbacks(Anvil::WINDOW_CALLBACK_ID_MOUSE_MOVED,
                                                                std::bind(&Engine::CallbackFunction_on_mouse_movement,
                                                                          this,
                                                                          std::placeholders::_1),
                                                                this);

    gameImport_uptr.reset();
    ECSwrapper_uptr.reset();
    inputManager.BindPlayerInputFreezeOldUnfreezeNew(nullptr);
    graphics_uptr.reset();
    camera_uptr.reset();
}

void Engine::CallbackFunction_on_close_event(Anvil::CallbackArgument*   in_callback_data_raw_ptr)
{
    breakMainLoop = true;
}

void Engine::CallbackFunction_on_keypress_was_up(Anvil::CallbackArgument*   in_callback_data_raw_ptr)
{
    auto callback_data_ptr = static_cast<Anvil::OnKeypressPressedWasUpCallbackArgument*>(in_callback_data_raw_ptr);

    inputManager.KeyPressed(callback_data_ptr->pressed_key_id);
}

void Engine::CallbackFunction_on_keypress_released(Anvil::CallbackArgument*   in_callback_data_raw_ptr)
{
    auto callback_data_ptr = static_cast<Anvil::OnKeypressReleasedCallbackArgument*>(in_callback_data_raw_ptr);

    inputManager.KeyReleased(callback_data_ptr->released_key_id);
}

void Engine::CallbackFunction_on_mouse_movement(Anvil::CallbackArgument*   in_callback_data_raw_ptr)
{
    auto callback_data_ptr = static_cast<Anvil::OnMouseMovementCallbackArgument*>(in_callback_data_raw_ptr);

    inputManager.MouseMoved(callback_data_ptr->xOffset, callback_data_ptr->yOffset);
}

Graphics* Engine::GetGraphicsPtr()
{
    return graphics_uptr.get();
}

ECSwrapper* Engine::GetECSwrapperPtr()
{
    return ECSwrapper_uptr.get();
}

void Engine::Run()
{
    while (!breakMainLoop)
    {
        for (auto this_event : inputManager.GrabAndResetEventVector())
        {
            switch (this_event)
            {
            case SHOULD_CLOSE:
                {
                    breakMainLoop = true;
                    break;
                }
            case TOGGLE_CULLING_DEBUG:
                {
                    camera_uptr->ToggleCullingDebugging();
                    break;
                }
            }
        }

        ECSwrapper_uptr->Update(true);
        graphics_uptr->DrawFrame();
    }
}