#include "ExportedFunctionsConstructor.h"

#include "Engine.h"

ExportedFunctionsConstructor::ExportedFunctionsConstructor(Engine* in_engine_ptr)
    :
    ExportedFunctions::ExportedFunctions(),
    engine_ptr(in_engine_ptr)
{
}

ExportedFunctionsConstructor::~ExportedFunctionsConstructor()
{
}

void ExportedFunctionsConstructor::BindCameraEntity(Entity this_camera_entity)
{
    CameraComp* cameraComp_ptr = reinterpret_cast<CameraComp*>(engine_ptr->GetECSwrapperPtr()->GetComponentByID(static_cast<componentID>(componentIDenum::Camera)));
    cameraComp_ptr->BindCameraEntity(this_camera_entity);
}
