#pragma once

#include "ECS/CompEntityBaseClass.h"

#include "ECS/ComponentBaseWrappedClass.h"

template<typename ComponentType>
class CompEntityBaseWrappedClass : public CompEntityBaseClass
{
public:
    explicit CompEntityBaseWrappedClass(const Entity this_entity)
        :CompEntityBaseClass(this_entity)
    {
    }

    static constexpr ComponentType GetComponentType();

protected:
    static ComponentType* GetComponentPtr()
    {
        return static_cast<ComponentType*>(__component_base_ptr);
    }

public:
    inline static ComponentBaseClass* __component_base_ptr = nullptr;
};