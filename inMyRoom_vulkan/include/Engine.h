#pragma once

#include <memory>
#include <string>

#include "configuru.hpp"

#include "GameImporter.h"
#include "ExportedFunctionsConstructor.h"
#include "InputManager.h"

#include "Graphics/VulkanInit.h"
#include "Graphics/Graphics.h"

#include "CollisionDetection/CollisionDetection.h"

#include "ECS/ECSwrapper.h"

class Engine: public VulkanInit
{
public:
    explicit Engine(configuru::Config& in_cfgFile);
    ~Engine();

    Graphics*       GetGraphicsPtr();
    GameImporter*   GetGameImporter();
    ECSwrapper*     GetECSwrapperPtr();

    void Run();

private: // data

    configuru::Config& cfgFile;

    std::unique_ptr<GameImporter> gameImporter_uptr;
    std::unique_ptr<Graphics> graphics_uptr;
    std::unique_ptr<CollisionDetection> collisionDetection_uptr;
    std::unique_ptr<ECSwrapper> ECSwrapper_uptr;
    std::unique_ptr<ExportedFunctionsConstructor> exportedFunctionsConstructor_uptr;

    std::unique_ptr<InputManager> inputManager_uptr;

    volatile bool breakMainLoop;
};
