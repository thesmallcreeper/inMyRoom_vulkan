#include "ComponentBaseClass.h"
#include "ECSwrapper.h"

ComponentBaseClass::ComponentBaseClass(const componentID this_ID, const std::string this_name, ECSwrapper* const in_ecs_wrapper_ptr)
    :thisComponentID(this_ID),
     thisComponentName(this_name),
     ecsWrapper_ptr(in_ecs_wrapper_ptr)
{
}

ComponentBaseClass::~ComponentBaseClass()
{
}

componentID ComponentBaseClass::GetComponentID() const
{
    return thisComponentID;
}

std::string ComponentBaseClass::GetComponentName() const
{
    return thisComponentName;
}

void ComponentBaseClass::InformEntitiesHandlerAboutAddition(const Entity this_entity) const
{
    ecsWrapper_ptr->GetEntitiesHandler()->EntityAttachedTo(this_entity, thisComponentID);
}

void ComponentBaseClass::InformEntitiesHandlerAboutRemoval(const Entity this_entity) const
{
    ecsWrapper_ptr->GetEntitiesHandler()->EntityDeattachFrom(this_entity, thisComponentID);
}
