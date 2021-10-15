#pragma once

#include "FixedString.h"
#include "ECS/ComponentsIDsEnum.h"

#include "ECS/ComponentBaseClass.h"

#include "ECS/CompEntityBaseClass.h"

#include "dense_set.h"
#include "sparse_set.h"

template<typename ComponentEntityType, componentID _component_ID, FixedString _component_name,
         template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
class ComponentBaseWrappedClass :
    public ComponentBaseClass
{
public:
    explicit ComponentBaseWrappedClass(ECSwrapper* in_ecs_wrapper_ptr);
    ~ComponentBaseWrappedClass() override;

    ComponentEntityType& GetComponentEntity(Entity this_entity);
    const ComponentEntityType& GetComponentEntity(Entity this_entity) const;

    static constexpr componentID component_ID = _component_ID;
    static constexpr FixedString component_name = _component_name;

    componentID GetComponentID() const final;
    std::string GetComponentName() const final;

    static constexpr ComponentEntityType GetComponentEntityType();
    static constexpr data_set<Entity, ComponentEntityType, CompEntityBaseClass, &CompEntityBaseClass::thisEntity> GetDataSetType();
};


// -----SOURCE-----

template<typename ComponentEntityType, componentID _component_ID, FixedString _component_name,
         template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
ComponentBaseWrappedClass<ComponentEntityType, _component_ID, _component_name, data_set>::ComponentBaseWrappedClass(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentBaseClass::ComponentBaseClass(in_ecs_wrapper_ptr)
{
    ComponentEntityType::__component_base_ptr = static_cast<ComponentBaseClass*>(this);
}

template<typename ComponentEntityType, componentID _component_ID, FixedString _component_name,
         template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
ComponentBaseWrappedClass<ComponentEntityType, _component_ID, _component_name, data_set>::~ComponentBaseWrappedClass()
{
    ComponentEntityType::__component_base_ptr = nullptr;
}

template<typename ComponentEntityType, componentID _component_ID, FixedString _component_name,
         template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
ComponentEntityType& ComponentBaseWrappedClass<ComponentEntityType, _component_ID, _component_name, data_set>::GetComponentEntity(const Entity this_entity)
{
    return *reinterpret_cast<ComponentEntityType*>(GetComponentEntityVoidPtr(this_entity));
}

template<typename ComponentEntityType, componentID _component_ID, FixedString _component_name,
         template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
const ComponentEntityType& ComponentBaseWrappedClass<ComponentEntityType, _component_ID, _component_name, data_set>::GetComponentEntity(const Entity this_entity) const
{
    return *reinterpret_cast<const ComponentEntityType*>(const_cast<ComponentBaseWrappedClass<ComponentEntityType, _component_ID, _component_name, data_set>*>(this)->GetComponentEntityVoidPtr(this_entity));
}

template<typename ComponentEntityType, componentID _component_ID, FixedString _component_name,
         template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
componentID ComponentBaseWrappedClass<ComponentEntityType, _component_ID, _component_name, data_set>::GetComponentID() const
{
    return _component_ID;
}

template<typename ComponentEntityType, componentID _component_ID, FixedString _component_name,
         template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
std::string ComponentBaseWrappedClass<ComponentEntityType, _component_ID, _component_name, data_set>::GetComponentName() const
{
    return std::string(_component_name.data(), _component_name.size());
}
