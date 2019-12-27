// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the GAMEDLL_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// GAMEDLL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef GAME_DLL
#define GAME_DLL_API __declspec(dllexport)
#else
#define GAME_DLL_API __declspec(dllimport)
#endif

#include "ECS/ECSwrapper.h"

GAME_DLL_API std::vector<std::unique_ptr<ComponentBaseClass>> __stdcall GetGameDLLComponents(ECSwrapper* const in_ecs_wrapper_ptr);
