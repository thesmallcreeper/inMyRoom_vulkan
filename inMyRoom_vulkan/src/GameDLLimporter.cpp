#include "GameDLLimporter.h"

GameDLLimporter::GameDLLimporter(ECSwrapper* in_ecs_wrapper_ptr, std::string gameDLL_path)
    :ECSwrapper_ptr(in_ecs_wrapper_ptr),
     gameDLLlib(nullptr)
{
    #ifdef _WIN32       // cause win32 is a moving cancer
    for (auto& this_char : gameDLL_path)
        if (this_char == '/')
            this_char = '\\';
    #endif

    #ifdef _WIN32
    gameDLLlib = ::LoadLibrary(gameDLL_path.c_str());
    #else
    gameDLLlib = ::dlopen(gameDLL_path.c_str(), RTLD_NOW);
    #endif
    assert(gameDLLlib != nullptr);

    #ifdef _WIN32
    FARPROC address = ::GetProcAddress(gameDLLlib, "GetGameDLLComponents");
    #else
    void* address = ::dlsym(gameDLLlib, "_Z20GetGameDLLComponentsP10ECSwrapper");
    #endif
    assert(address != nullptr);

    getGameDLLComponents_function = reinterpret_cast<std::vector<std::unique_ptr<ComponentBaseClass>>(STDCALL *)(ECSwrapper* const)>(address);
    assert(getGameDLLComponents_function != nullptr);
}

GameDLLimporter::~GameDLLimporter()
{
    gameDLLcomponents.clear();

    getGameDLLComponents_function = nullptr;

    #ifdef _WIN32
    ::FreeLibrary(gameDLLlib);
    #else
    dlclose(gameDLLlib);
    #endif
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
