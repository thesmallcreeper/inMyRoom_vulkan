#pragma once

#include "ECStypes.h"

enum class componentIDenum : componentID
{
    Default             = 0,
    Position            = 1,

    // Add scene specific IDs
    #ifdef GAME_DLL
    #include "GameSpecificComponentsIDsEnum.h"
    #endif

    NodeGlobalMatrix    = 2000000,
    ModelDraw           = 2000001
};