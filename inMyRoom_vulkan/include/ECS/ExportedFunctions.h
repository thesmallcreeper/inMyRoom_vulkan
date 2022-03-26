#pragma once

#include "ECS/ECStypes.h"

class ExportedFunctions
{
public:
    ExportedFunctions() {};
    virtual ~ExportedFunctions() {};

    // Virtual functions that export engines functions to .exe and .dll compoenents of ECS

    virtual void BindCameraEntity(Entity this_camera_entity) const = 0;

    virtual size_t GetSphereMeshIndex() const = 0;
    virtual size_t GetCylinderMeshIndex() const = 0;
};

