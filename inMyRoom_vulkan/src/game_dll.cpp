// game_dll.cpp : Defines the exported functions for the DLL.
//

#include "game_dll.h"
#include "GameSpecificComponentsVector.h"

GAME_DLL_API std::vector<std::unique_ptr<ComponentBaseClass>> GetGameDLLComponents(ECSwrapper* const in_ecs_wrapper_ptr)
{
    #pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
    return std::move(ConstructAndGetComponentsVector(in_ecs_wrapper_ptr));
}

