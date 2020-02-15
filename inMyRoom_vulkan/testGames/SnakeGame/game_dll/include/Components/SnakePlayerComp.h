#pragma once

#include "CompEntities/SnakePlayerCompEntity.h"
#include "ECS/ComponentSparseBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"

class SnakePlayerComp
    :public ComponentSparseBaseClass<SnakePlayerCompEntity>
{
public:
    SnakePlayerComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~SnakePlayerComp();

    void Update() override;
    void FixedUpdate() override {};
    void AsyncInput(InputType input_type, void* struct_data = nullptr) override;

    void CollisionCallback(const CollisionCallbackData& this_collisionCallbackData) override;

private:
    std::chrono::steady_clock::time_point lastSnapTimePoint;
};