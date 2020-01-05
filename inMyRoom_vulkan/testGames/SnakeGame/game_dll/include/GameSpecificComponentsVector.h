#pragma once

#include <vector>

#include "Components/VelocityComp.h"


std::vector<std::unique_ptr<ComponentBaseClass>> ConstructAndGetComponentsVector(ECSwrapper* const in_ecs_wrapper_ptr)
{
    std::vector<std::unique_ptr<ComponentBaseClass>> return_vector;

    {
        std::unique_ptr<VelocityComp> velocity_uptr = std::make_unique<VelocityComp>(in_ecs_wrapper_ptr);
        return_vector.emplace_back(std::move(velocity_uptr));
    }

    return std::move(return_vector);
}