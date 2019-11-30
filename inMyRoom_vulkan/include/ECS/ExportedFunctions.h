#pragma once

#include "ECS/ECStypes.h"

class ExportedFunctions
{
public:
    ExportedFunctions() {};
    virtual ~ExportedFunctions() {};

    // Virtual functions that export engines functions to .exe and .dll compoenents of ECS

    virtual Entity AddFabAndGetRoot(std::string fab_name, Entity parent_entity = 0, std::string preferred_name = "") = 0;
    virtual Entity AddFabAndGetRoot(std::string fab_name, std::string parent_path, std::string preferred_name = "") = 0;

    virtual Node* GetFabNode(std::string fab_node) = 0;

};

