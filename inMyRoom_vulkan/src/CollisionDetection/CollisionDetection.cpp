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
        createUncollideRays_uptr = std::make_unique<CreateUncollideRays>();
    }
    {
        shootDeltaUncollide_uptr = std::make_unique<ShootUncollideRays>(glm::radians(40.f),
                                                                        glm::radians(65.f),
                                                                        1.01f);
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

    // Mid phase collision (OBBtree vs OBBtree)
    std::vector<CDentriesPairTrianglesPairs> midPhaseResults;
    midPhaseResults.reserve(broadPhaseResults.size());
    for(const std::pair<CollisionDetectionEntry, CollisionDetectionEntry>& this_pair: broadPhaseResults)
    {
        CDentriesPairTrianglesPairs this_result = midPhaseCollision_uptr->ExecuteOBBtreesCollision(this_pair);
        if(this_result.OBBtreesIntersectInfoObj.candidateTriangleRangeCombinations.size())
        {
            midPhaseResults.emplace_back(std::move(this_result));
        }
    }     

    // Create rays phase (narrow phase)
    std::vector<CDentriesUncollideRays> createUncollideRaysResults;
    for (const CDentriesPairTrianglesPairs& this_triangles_pairs : midPhaseResults)
    {
        CDentriesUncollideRays this_result = createUncollideRays_uptr->ExecuteCreateUncollideRays(this_triangles_pairs);
        if(this_result.rays_from_first_to_second.size() || this_result.rays_from_second_to_first.size())
        {
            createUncollideRaysResults.emplace_back(std::move(this_result));
        }
    }

    std::unordered_map<Entity, std::vector<CollisionCallbackData>> callbacks_to_be_made;
    for (CDentriesUncollideRays& this_uncollideRaysResult : createUncollideRaysResults)
    {
        CollisionCallbackData first_collisionCallbackData;
        first_collisionCallbackData.familyEntity = this_uncollideRaysResult.firstEntry.entity;
        first_collisionCallbackData.collideWithEntity = this_uncollideRaysResult.secondEntry.entity;

        CollisionCallbackData second_collisionCallbackData;
        second_collisionCallbackData.familyEntity = this_uncollideRaysResult.secondEntry.entity;
        second_collisionCallbackData.collideWithEntity = this_uncollideRaysResult.firstEntry.entity;

        if(this_uncollideRaysResult.firstEntry.currentGlobalMatrix != this_uncollideRaysResult.firstEntry.previousGlobalMatrix ||
           this_uncollideRaysResult.secondEntry.currentGlobalMatrix != this_uncollideRaysResult.secondEntry.previousGlobalMatrix)
        {
            // Shoot the rays!
            glm::vec3 delta = shootDeltaUncollide_uptr->ExecuteShootUncollideRays(this_uncollideRaysResult);

            float first_movement_between_frames = PointMovementBetweenFrames(this_uncollideRaysResult.average_point_first_modelspace,
                                                                             this_uncollideRaysResult.firstEntry.currentGlobalMatrix,
                                                                             this_uncollideRaysResult.firstEntry.previousGlobalMatrix);

            float second_movement_between_frames = PointMovementBetweenFrames(this_uncollideRaysResult.average_point_second_modelspace,
                                                                              this_uncollideRaysResult.secondEntry.currentGlobalMatrix,
                                                                              this_uncollideRaysResult.secondEntry.previousGlobalMatrix);

            float total_movement = first_movement_between_frames + second_movement_between_frames;

            first_collisionCallbackData.deltaVector = - delta * (first_movement_between_frames / total_movement);
            second_collisionCallbackData.deltaVector = + delta * (second_movement_between_frames / total_movement);
        }
        else
        {
            first_collisionCallbackData.deltaVector = glm::vec3(0.f);
            second_collisionCallbackData.deltaVector = glm::vec3(0.f);
        }
        

        std::vector<Entity> first_ancestors = ECSwrapper_ptr->GetEntitiesHandler()->GetEntityAncestors(first_collisionCallbackData.familyEntity);
        std::vector<Entity> second_ancestors = ECSwrapper_ptr->GetEntitiesHandler()->GetEntityAncestors(second_collisionCallbackData.familyEntity);

        for(size_t index = 0; index != first_ancestors.size(); ++index)
        {
            if(index >= second_ancestors.size() ||
               first_ancestors[index] != second_ancestors[index])
            {
                callbacks_to_be_made[first_ancestors[index]].emplace_back(first_collisionCallbackData);
            }
        }

        for(size_t index = 0; index != second_ancestors.size(); ++index)
        {
            if(index >= first_ancestors.size() ||
               first_ancestors[index] != second_ancestors[index])
            {
                callbacks_to_be_made[second_ancestors[index]].emplace_back(second_collisionCallbackData);
            }
        }
    }

    MakeCallbacks(std::move(callbacks_to_be_made));
}

void CollisionDetection::MakeCallbacks(std::unordered_map<Entity, std::vector<CollisionCallbackData>>&& callbacks_to_be_made) const
{
    std::vector<std::pair<Entity, std::vector<CollisionCallbackData>>> callbacks_to_be_made_vector(std::make_move_iterator(callbacks_to_be_made.begin()),
                                                                                                   std::make_move_iterator(callbacks_to_be_made.end()));

    for(const auto& this_compID_component_ptr_pair: ECSwrapper_ptr->GetComponentIDtoComponentBaseClassMap())
    {
        if(this_compID_component_ptr_pair.second != nullptr)
            this_compID_component_ptr_pair.second->CollisionCallback(callbacks_to_be_made_vector);
    }
}

float CollisionDetection::PointMovementBetweenFrames(const glm::vec3 point, const glm::mat4& m_first, const glm::mat4& m_second)
{
    glm::vec3 p_first = glm::vec3(m_first * glm::vec4(point, 1.f));
    glm::vec3 p_second = glm::vec3(m_second * glm::vec4(point, 1.f));

    glm::vec3 v_diff = p_first - p_second;

    return glm::length(v_diff);
}