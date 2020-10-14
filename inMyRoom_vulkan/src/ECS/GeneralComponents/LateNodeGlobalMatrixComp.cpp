#include "ECS/GeneralComponents/LateNodeGlobalMatrixComp.h"

#include "ECS/ECSwrapper.h"

LateNodeGlobalMatrixComp::LateNodeGlobalMatrixComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentRawBaseClass<LateNodeGlobalMatrixCompEntity, static_cast<componentID>(componentIDenum::LateNodeGlobalMatrix), "LateNodeGlobalMatrix">(in_ecs_wrapper_ptr)
{
}

LateNodeGlobalMatrixComp::~LateNodeGlobalMatrixComp()
{
}
