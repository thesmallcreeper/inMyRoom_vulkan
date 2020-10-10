#include "ECS/ComponentBaseClass.h"
#include "ECS/ECSwrapper.h"

ComponentBaseClass::ComponentBaseClass(ECSwrapper* const in_ecs_wrapper_ptr)
    :ecsWrapper_ptr(in_ecs_wrapper_ptr)
{
}

ComponentBaseClass::~ComponentBaseClass()
{
}

ECSwrapper* ComponentBaseClass::GetECSwrapper() const
{
    return ecsWrapper_ptr;
}

void ComponentBaseClass::InformEntitiesHandlerAboutAddition(const Entity this_entity) const
{
    ecsWrapper_ptr->GetEntitiesHandler()->EntityAttachedTo(this_entity, GetComponentID());
}

void ComponentBaseClass::InformEntitiesHandlerAboutRemoval(const Entity this_entity) const
{
    ecsWrapper_ptr->GetEntitiesHandler()->EntityDeattachFrom(this_entity, GetComponentID());
}
