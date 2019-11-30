#pragma once

#include "ECS/ExportedFunctions.h"

class Engine;

class ExportedFunctionsConstructor
    : public ExportedFunctions
{
public:
    ExportedFunctionsConstructor(Engine* in_engine_ptr);
    ~ExportedFunctionsConstructor() override;

    Entity  AddFabAndGetRoot(std::string fab_name, Entity parent_entity = 0, std::string preferred_name = "") override;
    Entity  AddFabAndGetRoot(std::string fab_name, std::string parent_path, std::string preferred_name = "") override;

    Node*   GetFabNode(std::string fab_node) override;

private:
    Engine* const engine_ptr;
};