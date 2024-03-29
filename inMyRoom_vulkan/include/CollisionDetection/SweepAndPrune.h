#pragma once

#include "Geometry/OBBtree.h"
#include "ECS/ECStypes.h"

struct SweepAndPruneEntry
{
    std::pair<float, float> minMaxProjection;
    Entity index;
    bool shouldCallback;
};

struct hash_pair
{
    template <class T1, class T2>
    size_t operator()(const std::pair<T1, T2>& p) const
    {
        auto hash1 = std::hash<T1>{}(p.first);
        auto hash2 = std::hash<T2>{}(p.second);
        return hash1 ^ hash2;
    }
};

struct commutative_equal_pair
{
    template <class T1, class T2>
    bool operator()(const std::pair<T1, T2>& p1, const std::pair<T1, T2>& p2) const
    {
        return (p1.first == p2.first) && (p1.second == p2.second) ||
               (p1.first == p2.second) && (p1.second == p2.first);
    }
};

class SweepAndPrune
{
public:
    SweepAndPrune(glm::vec3 in_U_axis, glm::vec3 in_V_axis, glm::vec3 in_W_axis);

    std::vector<std::pair<CollisionDetectionEntry, CollisionDetectionEntry>> ExecuteSweepAndPrune(const std::vector<CollisionDetectionEntry>& collisionDetectionEntries);

private:
    const glm::vec3 U_axis;
    const glm::vec3 V_axis;
    const glm::vec3 W_axis;
};