#include "ECS/GeneralComponents/PositionComp.h"

#include "ECS/ECSwrapper.h"


PositionComp::PositionComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentRawBaseClass<PositionCompEntity>(static_cast<componentID>(componentIDenum::Position), "Position" , in_ecs_wrapper_ptr)
{
    PositionCompEntity::positionComp_ptr = this;
}

PositionComp::~PositionComp()
{
    PositionCompEntity::positionComp_ptr = nullptr;
}

std::vector<std::pair<std::string, MapType>> PositionComp::GetComponentInitMapFields()
{
    return PositionCompEntity::GetComponentInitMapFields();
}
