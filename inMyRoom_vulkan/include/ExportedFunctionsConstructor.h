#pragma once

#include "ECS/ExportedFunctions.h"

class Engine;

class ExportedFunctionsConstructor
    : public ExportedFunctions
{
public:
    ExportedFunctionsConstructor(Engine* in_engine_ptr);
    ~ExportedFunctionsConstructor() override;

    void    BindCameraEntity(Entity this_camera_entity) override;

private:
    Engine* const engine_ptr;
};