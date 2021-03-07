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
        rayDeltaUncollide_uptr = std::make_unique<RayDeltaUncollide>(30.f);
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
    std::vector<CSentriesPairTrianglesPairs> midPhaseResults;
    midPhaseResults.reserve(broadPhaseResults.size());
    for(const std::pair<CollisionDetectionEntry, CollisionDetectionEntry>& this_pair: broadPhaseResults)
    {
        CSentriesPairTrianglesPairs this_result = midPhaseCollision_uptr->ExecuteOBBtreesCollision(this_pair);
            if(this_result.OBBtreesIntersectInfo.candidateTriangleRangeCompinations.size())
            {
                midPhaseResults.emplace_back(std::move(this_result));
            }
    }     

    // Narrow phase collision
    std::vector<CSentriesPairCollisionCenter> narrowPhaseResults = narrowPhaseCollision_uptr->ExecuteTrianglesVsTriangles(midPhaseResults);

    std::vector<std::pair<Entity, CollisionCallbackData>> callbacks_to_be_made;
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

        std::vector<Entity> first_ancestors = ECSwrapper_ptr->GetEntitiesHandler()->GetEntityAncestors(first_collisionCallbackData.familyEntity);
        std::vector<Entity> second_ancestors = ECSwrapper_ptr->GetEntitiesHandler()->GetEntityAncestors(second_collisionCallbackData.familyEntity);

        for(size_t index = 0; index != first_ancestors.size(); ++index)
        {
            if(index >= second_ancestors.size() ||
               first_ancestors[index] != second_ancestors[index])
            {
                callbacks_to_be_made.emplace_back(first_ancestors[index], first_collisionCallbackData);
            }
        }

        for(size_t index = 0; index != second_ancestors.size(); ++index)
        {
            if(index >= first_ancestors.size() ||
               first_ancestors[index] != second_ancestors[index])
            {
                callbacks_to_be_made.emplace_back(second_ancestors[index], second_collisionCallbackData);
            }
        }
    }

    MakeCallbacks(callbacks_to_be_made);
}

void CollisionDetection::MakeCallbacks(const std::vector<std::pair<Entity, CollisionCallbackData>>& callback_entity_data_pairs) const
{
    for(const auto& this_compID_component_ptr_pair: ECSwrapper_ptr->GetComponentIDtoComponentBaseClassMap())
    {
        if(this_compID_component_ptr_pair.second != nullptr)
            this_compID_component_ptr_pair.second->CollisionCallback(callback_entity_data_pairs);
    }
}