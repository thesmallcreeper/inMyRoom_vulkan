#include "PositionComp.h"

#include "ECSwrapper.h"


PositionComp::PositionComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentRawBaseClass<PositionCompEntity>(static_cast<componentID>(componentIDenum::Position), "Position" , in_ecs_wrapper_ptr)
{
    PositionCompEntity::positionComp_ptr = this;
}

PositionComp::~PositionComp()
{
    PositionCompEntity::positionComp_ptr = nullptr;
}
