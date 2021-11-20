#include "ECS/GeneralComponents/LateNodeGlobalMatrixComp.h"

#include "ECS/ECSwrapper.h"

LateNodeGlobalMatrixComp::LateNodeGlobalMatrixComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentDataClass<LateNodeGlobalMatrixCompEntity, static_cast<componentID>(componentIDenum::LateNodeGlobalMatrix), "LateNodeGlobalMatrix", dense_set>(in_ecs_wrapper_ptr)
{
}

LateNodeGlobalMatrixComp::~LateNodeGlobalMatrixComp()
{
}

