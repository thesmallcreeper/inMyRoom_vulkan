#pragma once

#include "ECS/ECStypes.h"

enum class componentIDenum : componentID
{
    // Add scene specific IDs
    #ifdef GAME_DLL
    #include "GameSpecificComponentsIDsEnum.h"
    #endif

    Default             = 0,
    Position            = 1000,
    NodeGlobalMatrix    = 2000,
    DefaultCameraInput  = 3000,
    Camera              = 4000,
    ModelDraw           = 5000
};