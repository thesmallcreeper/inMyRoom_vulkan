#include "ECS/GeneralComponents/SkinComp.h"

#include "ECS/ECSwrapper.h"

SkinComp::SkinComp(ECSwrapper* const in_ecs_wrapper_ptr, SkinsOfMeshes* in_skinsOfMeshes_ptr)
    :ComponentDataClass<SkinCompEntity, static_cast<componentID>(componentIDenum::Skin), "Skin", sparse_set>(in_ecs_wrapper_ptr),
     skinsOfMeshes_ptr(in_skinsOfMeshes_ptr)
{
}
