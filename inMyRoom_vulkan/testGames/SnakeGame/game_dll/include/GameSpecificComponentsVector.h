#pragma once

#include <vector>

#include "Components/SnakePlayerComp.h"


std::vector<std::unique_ptr<ComponentBaseClass>> ConstructAndGetComponentsVector(ECSwrapper* const in_ecs_wrapper_ptr)
{
    std::vector<std::unique_ptr<ComponentBaseClass>> return_vector;

    {
        std::unique_ptr<SnakePlayerComp> snakePlayer_uptr = std::make_unique<SnakePlayerComp>(in_ecs_wrapper_ptr);
        return_vector.emplace_back(std::move(snakePlayer_uptr));
    }

    return std::move(return_vector);
}