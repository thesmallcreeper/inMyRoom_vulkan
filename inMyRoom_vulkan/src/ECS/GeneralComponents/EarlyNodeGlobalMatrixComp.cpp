#include "ECS/GeneralComponents/EarlyNodeGlobalMatrixComp.h"

#include "ECS/ECSwrapper.h"

EarlyNodeGlobalMatrixComp::EarlyNodeGlobalMatrixComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentDataClass<EarlyNodeGlobalMatrixCompEntity, static_cast<componentID>(componentIDenum::EarlyNodeGlobalMatrix), "EarlyNodeGlobalMatrix", dense_set>(in_ecs_wrapper_ptr)
{
}

EarlyNodeGlobalMatrixComp::~EarlyNodeGlobalMatrixComp()
{
}

