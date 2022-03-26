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

void ExportedFunctionsConstructor::BindCameraEntity(Entity this_camera_entity) const
{
    auto* cameraComp_ptr = static_cast<CameraComp*>(engine_ptr->GetECSwrapperPtr()->GetComponentByID(static_cast<componentID>(componentIDenum::Camera)));
    cameraComp_ptr->BindCameraEntity(this_camera_entity);
}

size_t ExportedFunctionsConstructor::GetSphereMeshIndex() const
{
    size_t mesh_index = engine_ptr->GetGraphicsPtr()->GetMeshesOfNodesPtr()->GetSphereMeshIndex();
    return mesh_index;
}

size_t ExportedFunctionsConstructor::GetCylinderMeshIndex() const
{
    size_t mesh_index = engine_ptr->GetGraphicsPtr()->GetMeshesOfNodesPtr()->GetCylinderMeshIndex();
    return mesh_index;
}


