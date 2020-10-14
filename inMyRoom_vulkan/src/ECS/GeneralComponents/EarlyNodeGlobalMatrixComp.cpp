#include "ECS/GeneralComponents/EarlyNodeGlobalMatrixComp.h"

#include "ECS/ECSwrapper.h"

EarlyNodeGlobalMatrixComp::EarlyNodeGlobalMatrixComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentRawBaseClass<EarlyNodeGlobalMatrixCompEntity, static_cast<componentID>(componentIDenum::EarlyNodeGlobalMatrix), "EarlyNodeGlobalMatrix">(in_ecs_wrapper_ptr)
{
}

EarlyNodeGlobalMatrixComp::~EarlyNodeGlobalMatrixComp()
{
}
