#include "CollisionDetection/CollisionDetection.h"

#include <algorithm>

CollisionDetection::CollisionDetection(ECSwrapper* in_ECSwrapper_ptr)
    :ECSwrapper_ptr(in_ECSwrapper_ptr)
{
    {
        glm::vec3 U_axis = glm::normalize(glm::vec3(0.8f, -0.2f, 0.f));
        glm::vec3 W_axis = glm::normalize(glm::cross(U_axis, glm::vec3(0.f, -1.f, 0.f)));
        glm::vec3 V_axis = glm::normalize(glm::cross(W_axis, U_axis));

        broadPhaseCollision_uptr = std::make_unique<SweepAndPrune>(U_axis, V_axis, W_axis);
    }
    {
        midPhaseCollision_uptr = std::make_unique<OBBtreesCollision>();
    }
    {
        narrowPhaseCollision_uptr = std::make_unique<TrianglesVsTriangles>();
    }
    {
        rayDeltaUncollide_uptr = std::make_unique<RayDeltaUncollide>(5.f);
    }
}

void CollisionDetection::Reset()
{
    collisionDetectionEntries.clear();
}

void CollisionDetection::AddCollisionDetectionEntry(const CollisionDetectionEntry in_collisionDetectionEntry)
{
    collisionDetectionEntries.emplace_back(in_collisionDetectionEntry);
}

void CollisionDetection::ExecuteCollisionDetection()
{
    if (collisionDetectionEntries.size() < 2) return;

    // Broad phase collision
    // at least one of the entries should have callback
    std::vector<std::pair<CollisionDetectionEntry, CollisionDetectionEntry>> broadPhaseResults = broadPhaseCollision_uptr->ExecuteSweepAndPrune(collisionDetectionEntries);

    // Mid phase collision
    std::vector<CSentriesPairTrianglesPairs> midPhaseResults = midPhaseCollision_uptr->ExecuteOBBtreesCollision(broadPhaseResults);

    // Narrow phase collision
    std::vector<CSentriesPairCollisionCenter> narrowPhaseResults = narrowPhaseCollision_uptr->ExecuteTrianglesVsTriangles(midPhaseResults);

    for (CSentriesPairCollisionCenter& this_narrowPhaseResult : narrowPhaseResults)
    {
        if (this_narrowPhaseResult.firstEntry.entity > this_narrowPhaseResult.secondEntry.entity)
        {
            std::swap(this_narrowPhaseResult.firstEntry, this_narrowPhaseResult.secondEntry);
        }

        CollisionCallbackData first_collisionCallbackData;
        first_collisionCallbackData.familyEntity = this_narrowPhaseResult.firstEntry.entity;
        first_collisionCallbackData.collideWithEntity = this_narrowPhaseResult.secondEntry.entity;

        CollisionCallbackData second_collisionCallbackData;
        second_collisionCallbackData.familyEntity = this_narrowPhaseResult.secondEntry.entity;
        second_collisionCallbackData.collideWithEntity = this_narrowPhaseResult.firstEntry.entity;

        if(this_narrowPhaseResult.firstEntry.currentGlobalMatrix != this_narrowPhaseResult.firstEntry.previousGlobalMatrix ||
           this_narrowPhaseResult.secondEntry.currentGlobalMatrix != this_narrowPhaseResult.secondEntry.previousGlobalMatrix)
        {
            std::pair<glm::vec3, glm::vec3> delta = rayDeltaUncollide_uptr->ExecuteRayDeltaUncollide(this_narrowPhaseResult);

            first_collisionCallbackData.deltaVector = delta.first;
            second_collisionCallbackData.deltaVector = delta.second;
        }

        Entity first_entity_ancestor = first_collisionCallbackData.familyEntity;
        Entity second_entity_ancestor = second_collisionCallbackData.familyEntity;
        while (first_entity_ancestor != second_entity_ancestor)
        {
            while (second_entity_ancestor > first_entity_ancestor)
            {
                if(this_narrowPhaseResult.secondEntry.shouldCallback)
                    CallbackEntity(second_entity_ancestor, second_collisionCallbackData);

                second_entity_ancestor = ECSwrapper_ptr->GetEntitiesHandler()->GetParentOfEntity(second_entity_ancestor);
            }

            if (first_entity_ancestor != second_entity_ancestor)
            {
                if (this_narrowPhaseResult.firstEntry.shouldCallback)
                    CallbackEntity(first_entity_ancestor, first_collisionCallbackData);

                first_entity_ancestor = ECSwrapper_ptr->GetEntitiesHandler()->GetParentOfEntity(first_entity_ancestor);
            }
        }
    }
}

void CollisionDetection::CallbackEntity(Entity this_entity, const CollisionCallbackData& collision_callback_data)
{
    std::vector<componentID> components_of_entity = ECSwrapper_ptr->GetEntitiesHandler()->GetComponentsOfEntity(this_entity);
    for (const componentID this_component_of_entity_ID : components_of_entity)
    {
        ComponentBaseClass* this_component_of_entity_ptr = ECSwrapper_ptr->GetComponentByID(this_component_of_entity_ID);
        this_component_of_entity_ptr->CollisionCallback(collision_callback_data);
    }
}
