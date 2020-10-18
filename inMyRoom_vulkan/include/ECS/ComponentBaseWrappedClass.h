#pragma once

#include "FixedString.h"
#include "ECS/ComponentsIDsEnum.h"

#include "ECS/ComponentBaseClass.h"

template<typename ComponentEntityType, componentID _component_ID, FixedString _component_name>
class ComponentBaseWrappedClass :
    public ComponentBaseClass
{
public:
    ComponentBaseWrappedClass(ECSwrapper* const in_ecs_wrapper_ptr);
    ~ComponentBaseWrappedClass();

    ComponentEntityType& GetComponentEntity(const Entity this_entity);
    const ComponentEntityType& GetComponentEntity(const Entity this_entity) const;

    static constexpr componentID component_ID = _component_ID;
    static constexpr FixedString component_name = _component_name;

    componentID GetComponentID() const final;
    std::string GetComponentName() const final;

    static constexpr ComponentEntityType GetComponentEntityType();
};


// -----SOURCE-----

template<typename ComponentEntityType, componentID _component_ID, FixedString _component_name>
inline ComponentBaseWrappedClass<ComponentEntityType, _component_ID, _component_name>::ComponentBaseWrappedClass(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentBaseClass::ComponentBaseClass(in_ecs_wrapper_ptr)
{
    ComponentEntityType::__component_base_ptr = static_cast<ComponentBaseClass*>(this);
}

template <typename ComponentEntityType, componentID _component_ID, FixedString _component_name>
inline ComponentBaseWrappedClass<ComponentEntityType, _component_ID, _component_name>::~ComponentBaseWrappedClass()
{
    ComponentEntityType::__component_base_ptr = nullptr;
}

template <typename ComponentEntityType, componentID _component_ID, FixedString _component_name>
ComponentEntityType& ComponentBaseWrappedClass<ComponentEntityType, _component_ID, _component_name>::GetComponentEntity(const Entity this_entity)
{
    return *reinterpret_cast<ComponentEntityType*>(GetComponentEntityVoidPtr(this_entity));
}

template <typename ComponentEntityType, componentID _component_ID, FixedString _component_name>
const ComponentEntityType& ComponentBaseWrappedClass<ComponentEntityType, _component_ID, _component_name>::GetComponentEntity(const Entity this_entity) const
{
    return *reinterpret_cast<const ComponentEntityType*>(const_cast<ComponentBaseWrappedClass<ComponentEntityType, _component_ID, _component_name>*>(this)->GetComponentEntityVoidPtr(this_entity));
}

template<typename ComponentEntityType, componentID _component_ID, FixedString _component_name>
inline componentID ComponentBaseWrappedClass<ComponentEntityType, _component_ID, _component_name>::GetComponentID() const
{
    return _component_ID;
}

template<typename ComponentEntityType, componentID _component_ID, FixedString _component_name>
inline std::string ComponentBaseWrappedClass<ComponentEntityType, _component_ID, _component_name>::GetComponentName() const
{
    return std::string(_component_name.data(), _component_name.size());
}
