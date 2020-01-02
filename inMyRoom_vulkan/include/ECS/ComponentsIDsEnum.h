#pragma once

#include "ECS/ECStypes.h"

enum class componentIDenum : componentID
{
    // Add scene specific IDs
    #ifdef GAME_DLL
    #include "GameSpecificComponentsIDsEnum.inj"
    #endif

    Default             = 0,
    AnimationComposer   = 1000,
    AnimationActor      = 2000,
    Position            = 3000,
    NodeGlobalMatrix    = 4000,
    Skin                = 5000,
    DefaultCameraInput  = 6000,
    Camera              = 7000,
    ModelDraw           = 8000  // is custom
};