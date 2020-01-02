#pragma once

#include "ECS/ECStypes.h"

enum class componentIDenum : componentID
{
    // Add scene specific IDs
    #ifdef GAME_DLL
    #include "GameSpecificComponentsIDsEnum.inj"
    #endif

    Default             = 0,
    NodeData            = 1000,
    AnimationComposer   = 2000,
    AnimationActor      = 3000,
    NodeGlobalMatrix    = 4000,
    Skin                = 5000,
    CameraDefaultInput  = 6000,
    Camera              = 7000,
    ModelDraw           = 8000  // is custom
};