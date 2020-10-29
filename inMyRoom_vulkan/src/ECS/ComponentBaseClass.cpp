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
