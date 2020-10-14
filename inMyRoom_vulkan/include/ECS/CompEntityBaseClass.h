#pragma once

#include "ECStypes.h"

class CompEntityBaseClass 
{
public:
    CompEntityBaseClass(const Entity this_entity)
        :thisEntity(this_entity)
    {
    }

    Entity thisEntity;
};