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

Entity ExportedFunctionsConstructor::AddFabAndGetRoot(std::string fab_name, Entity parent_entity, std::string preferred_name)
{
    return engine_ptr->GetGameImporter()->AddFabAndGetRoot(fab_name, parent_entity, preferred_name);
}

Entity ExportedFunctionsConstructor::AddFabAndGetRoot(std::string fab_name, std::string parent_path, std::string preferred_name)
{
    return engine_ptr->GetGameImporter()->AddFabAndGetRoot(fab_name, parent_path, preferred_name);
}

Node* ExportedFunctionsConstructor::GetFabNode(std::string fab_node)
{
    return engine_ptr->GetGameImporter()->GetFabNode(fab_node);
}