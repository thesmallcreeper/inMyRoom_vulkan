#include "ECS/GeneralComponents/DynamicMeshComp.h"

#include "ECS/ECSwrapper.h"

DynamicMeshComp::DynamicMeshComp(ECSwrapper* const in_ecs_wrapper_ptr, SkinsOfMeshes* in_skinsOfMeshes_ptr)
    :ComponentDataClass<DynamicMeshCompEntity, static_cast<componentID>(componentIDenum::DynamicMesh), "DynamicMesh", sparse_set>(in_ecs_wrapper_ptr),
     skinsOfMeshes_ptr(in_skinsOfMeshes_ptr)
{
}
