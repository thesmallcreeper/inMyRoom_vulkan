// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the GAMEDLL_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// GAMEDLL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

// https://stackoverflow.com/a/2164853
#if defined(_MSC_VER)
    //  Microsoft 
#define EXPORT __declspec(dllexport)
#define IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
    //  GCC
#define EXPORT __attribute__((visibility("default")))
#define IMPORT
#else
    //  do nothing and hope for the best?
#define EXPORT
#define IMPORT
#pragma warning Unknown dynamic link import/export semantics.
#endif

#ifdef GAME_DLL
#define GAME_DLL_API EXPORT
#else
#define GAME_DLL_API IMPORT
#endif

#ifdef _WIN32
#define STDCALL __stdcall
#else
#define STDCALL __attribute__((stdcall))
#endif


#include "ECS/ECSwrapper.h"

GAME_DLL_API std::vector<std::unique_ptr<ComponentBaseClass>> STDCALL GetGameDLLComponents(ECSwrapper* const in_ecs_wrapper_ptr);
