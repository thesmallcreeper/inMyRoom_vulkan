#pragma once

#include <functional>
#include <vector>

#include <windows.h>

#include "ECS/ECSwrapper.h"

class GameDLLimporter
{
public:
    GameDLLimporter(ECSwrapper* in_ecs_wrapper_ptr, std::string gameDLL_path);
    ~GameDLLimporter();
    
    void AddComponentsToECSwrapper();

private:
    std::vector<std::unique_ptr<ComponentBaseClass>> gameDLLcomponents;

    HINSTANCE gameDLLlib;
    std::function<std::vector<std::unique_ptr<ComponentBaseClass>>(ECSwrapper* const)> getGameDLLComponents_function;

    ECSwrapper* const ECSwrapper_ptr;
};