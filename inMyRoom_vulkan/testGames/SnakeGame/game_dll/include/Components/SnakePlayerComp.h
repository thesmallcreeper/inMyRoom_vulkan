#pragma once

#include "CompEntities/SnakePlayerCompEntity.h"
#include "ECS/ComponentDataClass.h"

#include "ECS/ComponentsIDsEnum.h"

class SnakePlayerComp
    :public ComponentDataClass<SnakePlayerCompEntity, static_cast<componentID>(componentIDenum::SnakePlayer), "SnakePlayer", sparse_set>
{
public:
    SnakePlayerComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~SnakePlayerComp();

    void Update() override;
    void AsyncInput(InputType input_type, void* struct_data = nullptr) override;

    void CollisionCallback(const std::vector<std::pair<Entity, std::vector<CollisionCallbackData>>>& callback_entity_data_pairs) override;

private:
    std::chrono::steady_clock::time_point lastSnapTimePoint;
};