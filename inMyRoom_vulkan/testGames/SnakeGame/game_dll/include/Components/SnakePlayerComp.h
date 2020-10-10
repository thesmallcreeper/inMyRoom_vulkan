#pragma once

#include "CompEntities/SnakePlayerCompEntity.h"
#include "ECS/ComponentSparseBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"

class SnakePlayerComp
    :public ComponentSparseBaseClass<SnakePlayerCompEntity, static_cast<componentID>(componentIDenum::SnakePlayer), "SnakePlayer">
{
public:
    SnakePlayerComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~SnakePlayerComp();

    void Update() override;
    void AsyncInput(InputType input_type, void* struct_data = nullptr) override;

    void CollisionCallback(Entity this_entity, const CollisionCallbackData& this_collisionCallbackData) override;

private:
    std::chrono::steady_clock::time_point lastSnapTimePoint;
};