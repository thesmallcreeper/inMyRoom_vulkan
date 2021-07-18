#pragma once

#include <functional>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#ifdef _WIN32
#define STDCALL __stdcall
#else
#define STDCALL __attribute__((stdcall))
#endif

#include "ECS/ECSwrapper.h"

class GameDLLimporter
{
public:
    GameDLLimporter(ECSwrapper* in_ecs_wrapper_ptr, std::string gameDLL_path);
    ~GameDLLimporter();
    
    void AddComponentsToECSwrapper();

private:
    std::vector<std::unique_ptr<ComponentBaseClass>> gameDLLcomponents;

    #ifdef _WIN32
    HINSTANCE gameDLLlib;
    #else
    void* gameDLLlib;
    #endif
    std::function<std::vector<std::unique_ptr<ComponentBaseClass>>(ECSwrapper* const)> getGameDLLComponents_function;

    ECSwrapper* const ECSwrapper_ptr;
};