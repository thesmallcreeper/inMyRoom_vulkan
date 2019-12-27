#include "Components/VelocityComp.h"

#include "ECS/ECSwrapper.h" 

VelocityComp::VelocityComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentSparseBaseClass<VelocityCompEntity>(static_cast<componentID>(componentIDenum::Velocity), "Velocity", in_ecs_wrapper_ptr)
{
    VelocityCompEntity::velocityComp_ptr = this;
}

VelocityComp::~VelocityComp()
{
    VelocityCompEntity::velocityComp_ptr = nullptr;
}

void VelocityComp::Update()
{
    componentID position_componentID = static_cast<componentID>(componentIDenum::Position);
    ComponentBaseClass* const positionComp_bptr = ecsWrapper_ptr->GetComponentByID(position_componentID);   

    std::chrono::duration<float> delta_time = ecsWrapper_ptr->GetUpdateDeltaTime();

    for (size_t index = 0; index < componentEntitiesSparse.size(); index++)
        componentEntitiesSparse[index].Update(positionComp_bptr, delta_time);
}
