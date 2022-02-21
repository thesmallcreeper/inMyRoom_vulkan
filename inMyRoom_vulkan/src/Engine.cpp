#include "Engine.h"

#include <iostream>

#include "InputManager.h"
#include "configuru.hpp"

#include "ECS/GeneralComponents/AnimationComposerComp.h"
#include "ECS/GeneralComponents/NodeDataComp.h"
#include "ECS/GeneralComponents/EarlyNodeGlobalMatrixComp.h"
#include "ECS/GeneralComponents/ModelCollisionComp.h"
#include "ECS/GeneralComponents/LateNodeGlobalMatrixComp.h"
#include "ECS/GeneralComponents/CameraDefaultInputComp.h"

Engine::Engine(configuru::Config& in_cfgFile)
    :
    VulkanInit(in_cfgFile),
    cfgFile(in_cfgFile),
    breakMainLoop(false)
{
    {   // Initializing engine exported functions
        exportedFunctionsConstructor_uptr = std::make_unique<ExportedFunctionsConstructor>(this);
    }

    {   // Initializing ECS
        ECSwrapper_uptr = std::make_unique<ECSwrapper>(exportedFunctionsConstructor_uptr.get());

        std::unique_ptr<AnimationComposerComp> animationComposer_comp_uptr = std::make_unique<AnimationComposerComp>(ECSwrapper_uptr.get());
        ECSwrapper_uptr->AddComponentAndOwnership(std::move(animationComposer_comp_uptr));

        std::unique_ptr<NodeDataComp> position_comp_uptr= std::make_unique<NodeDataComp>(ECSwrapper_uptr.get());
        ECSwrapper_uptr->AddComponentAndOwnership(std::move(position_comp_uptr));

        std::unique_ptr<EarlyNodeGlobalMatrixComp> earlyNodeGlobalMatrix_comp_uptr = std::make_unique<EarlyNodeGlobalMatrixComp>(ECSwrapper_uptr.get());
        ECSwrapper_uptr->AddComponentAndOwnership(std::move(earlyNodeGlobalMatrix_comp_uptr));

        std::unique_ptr<LateNodeGlobalMatrixComp> lateNodeGlobalMatrix_comp_uptr = std::make_unique<LateNodeGlobalMatrixComp>(ECSwrapper_uptr.get());
        ECSwrapper_uptr->AddComponentAndOwnership(std::move(lateNodeGlobalMatrix_comp_uptr));

        std::unique_ptr<CameraDefaultInputComp> defaultCameraInput_comp_uptr = std::make_unique<CameraDefaultInputComp>(ECSwrapper_uptr.get(), cfgFile["DefaultCamera"]["Speed"].as_float());
        ECSwrapper_uptr->AddComponentAndOwnership(std::move(defaultCameraInput_comp_uptr));

    }

    {   // Initializing graphics which add some specific componentsCount to ECS
        graphics_uptr = std::make_unique<Graphics>(this, cfgFile, device, vma_allocator);
    }

    {   // Initializing collision detection
        collisionDetection_uptr = std::make_unique<CollisionDetection>(ECSwrapper_uptr.get());

        std::unique_ptr<ModelCollisionComp> modelCollision_comp_uptr = std::make_unique<ModelCollisionComp>(ECSwrapper_uptr.get(), collisionDetection_uptr.get(), graphics_uptr->GetMeshesOfNodesPtr());
        ECSwrapper_uptr->AddComponentAndOwnership(std::move(modelCollision_comp_uptr));
    }

    {   // Game importing
        std::string game_file_path = cfgFile["game"]["path"].as_string();
        gameImporter_uptr = std::make_unique<GameImporter>(this, game_file_path);
    }

    {   // Initializing input manager
        inputManager_uptr = std::make_unique<InputManager>(this, cfgFile);
    }

    {   // Enabling mouse/keyboard input
        GetWindowPtr()->AddCallbackKeyPressLambda([this](int key) {this->inputManager_uptr->KeyPressed(key);});
        GetWindowPtr()->AddCallbackKeyReleaseLambda([this](int key) {this->inputManager_uptr->KeyReleased(key);});
        GetWindowPtr()->AddCallbackMouseMoveLambda([this](long dx, long dy) {this->inputManager_uptr->MouseMoved(dx, dy);});
    }

    std::cout << "Live!\n";
}

Engine::~Engine()
{
    GetWindowPtr()->DeleteCallbacks();

    inputManager_uptr.reset();
    ECSwrapper_uptr.reset();
    gameImporter_uptr.reset();
    graphics_uptr.reset();
    collisionDetection_uptr.reset();
    exportedFunctionsConstructor_uptr.reset();
}

Graphics* Engine::GetGraphicsPtr()
{
    return graphics_uptr.get();
}

GameImporter* Engine::GetGameImporter()
{
    return gameImporter_uptr.get();
}

ECSwrapper* Engine::GetECSwrapperPtr()
{
    return ECSwrapper_uptr.get();
}

void Engine::Run()
{
    ECSwrapper_uptr->RefreshUpdateDeltaTime();  // In order to make 1st frame delta time about 0.

    while (!breakMainLoop)
    {
        for (auto this_event : inputManager_uptr->GrabAndResetEventVector())
        {
            switch (this_event)
            {
                case eventInputIDenums::SHOULD_CLOSE:
                {
                    breakMainLoop = true;
                    break;
                }
                case eventInputIDenums::TOGGLE_CULLING_DEBUG:
                {
                    graphics_uptr->ToggleCullingDebugging();
                    break;
                }
                case eventInputIDenums::TOGGLE_VIEWPORT_FREEZE:
                {
                    graphics_uptr->ToggleViewportFreeze();
                    break;
                }
            }
        }

        if (this->GetWindowPtr()->ShouldClose()) {
            breakMainLoop = true;
        }

        ECSwrapper_uptr->Update();
        graphics_uptr->DrawFrame();
        ECSwrapper_uptr->CompleteAddsAndRemoves();
    }
}