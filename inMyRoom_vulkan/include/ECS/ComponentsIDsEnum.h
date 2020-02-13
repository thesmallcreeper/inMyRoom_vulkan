#pragma once

#include "ECS/ECStypes.h"

enum class componentIDenum : componentID
{
    // Add scene specific IDs
    #ifdef GAME_DLL
    #include "GameSpecificComponentsIDsEnum.inj"
    #endif

    Default                 = 0,
    NodeData                = 1000,
    AnimationComposer       = 2000,
    AnimationActor          = 3000,
    EarlyNodeGlobalMatrix   = 4000,
    LateNodeGlobalMatrix    = 5000,
    Skin                    = 6000,
    CameraDefaultInput      = 7000,
    Camera                  = 8000,
    ModelDraw               = 9000  // is custom
};