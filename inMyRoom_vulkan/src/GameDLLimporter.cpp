#include "GameDLLimporter.h"

GameDLLimporter::GameDLLimporter(ECSwrapper* in_ecs_wrapper_ptr, std::string gameDLL_path)
    :ECSwrapper_ptr(in_ecs_wrapper_ptr),
     gameDLLlib(nullptr)
{
    for (auto& this_char : gameDLL_path)
        if (this_char == '/')
            this_char = '\\';

    gameDLLlib = ::LoadLibrary(gameDLL_path.c_str());
    assert(gameDLLlib != nullptr);

    getGameDLLComponents_function = reinterpret_cast<std::vector<std::unique_ptr<ComponentBaseClass>>(__stdcall *)(ECSwrapper* const)>(::GetProcAddress(gameDLLlib, "GetGameDLLComponents"));
    assert(getGameDLLComponents_function != nullptr);
}

GameDLLimporter::~GameDLLimporter()
{
    gameDLLcomponents.clear();

    getGameDLLComponents_function = nullptr;
    ::FreeLibrary(gameDLLlib);
}

void GameDLLimporter::AddComponentsToECSwrapper()
{
    assert(getGameDLLComponents_function);
    gameDLLcomponents = getGameDLLComponents_function(ECSwrapper_ptr);

    for (size_t index = 0; index < gameDLLcomponents.size(); index++)
    {
        printf("--Importing component: %s \n", gameDLLcomponents[index]->GetComponentName().c_str());
        ECSwrapper_ptr->AddComponent(gameDLLcomponents[index].get());        
    }
}
