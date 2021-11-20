#pragma once

#include "ECS/ECStypes.h"

enum class componentIDenum : componentID
{
    // Add scene specific IDs
    #ifdef GAME_DLL
    #include "GameSpecificComponentsIDsEnum.inj"
    #endif

    Default                 =     0,
    NodeData                =  1000,
    AnimationComposer       =  2000,
    AnimationActor          =  3000,
    EarlyNodeGlobalMatrix   =  4000,
    ModelCollision          =  5000,
    LateNodeGlobalMatrix    =  6000,
    Skin                    =  7000,
    CameraDefaultInput      =  8000,
    Camera                  =  9000,
    ModelDraw               = 10000,  // is custom

    DefaultLast             = componentID(-1)
};