#pragma once

#include "ECS/ComponentBaseWrappedClass.h"

template<typename ComponentType>
class CompEntityBase
{
public:
    CompEntityBase(const Entity this_entity)
        :thisEntity(this_entity)
    {
    }

    static constexpr ComponentType GetComponentType();

    Entity thisEntity;

protected:
    static ComponentType* GetComponentPtr()
    {
        return static_cast<ComponentType*>(__component_base_ptr);
    }

public:
    inline static ComponentBaseClass* __component_base_ptr = nullptr;
};