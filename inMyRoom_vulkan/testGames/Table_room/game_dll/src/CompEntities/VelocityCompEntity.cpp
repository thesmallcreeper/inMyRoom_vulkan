#include "CompEntities/VelocityCompEntity.h"

#include "ECS/ECSwrapper.h"
#include "ECS/GeneralCompEntities/PositionCompEntity.h"

VelocityComp* VelocityCompEntity::velocityComp_ptr = nullptr;

VelocityCompEntity::VelocityCompEntity(const Entity this_entity)
    :thisEntity(this_entity)
{
}

VelocityCompEntity::~VelocityCompEntity()
{
}

VelocityCompEntity VelocityCompEntity::GetEmpty()
{
    VelocityCompEntity this_velocityEntity(0);

    return this_velocityEntity;
}

VelocityCompEntity VelocityCompEntity::CreateComponentEntityByMap(const Entity in_entity, const CompEntityInitMap in_map)
{
    VelocityCompEntity this_velocityEntity(in_entity);

    // "Velocity", localScale.xyz = vec4.xyz (optional)

    {
        auto search = in_map.vec4Map.find("Velocity");
        if (search != in_map.vec4Map.end())
        {
            glm::vec4 this_vec4 = search->second;
            this_velocityEntity.velocity = glm::vec3(this_vec4.x, this_vec4.y, this_vec4.z);
        }
    }

    return this_velocityEntity;
}

std::vector<std::pair<std::string, MapType>> VelocityCompEntity::GetComponentInitMapFields()
{
    std::vector<std::pair<std::string, MapType>> return_pair;
    return_pair.emplace_back(std::make_pair("Velocity", MapType::vec3_type));

    return return_pair;
}

void VelocityCompEntity::Update(ComponentBaseClass* positionComp_bptr, std::chrono::duration<float> deltaTime)
{
    PositionCompEntity* this_position_componentEntity_ptr = reinterpret_cast<PositionCompEntity*>(positionComp_bptr->GetComponentEntity(thisEntity));

    this_position_componentEntity_ptr->GlobalTranslate(deltaTime.count() * velocity);
}

void VelocityCompEntity::Init()
{
}
