#include "CollisionDetection/OBBtreesCollision.h"

OBBtreesCollision::OBBtreesCollision()
{
}

std::vector<CSentriesPairTrianglesPairs> OBBtreesCollision::ExecuteOBBtreesCollision(const std::vector<std::pair<CollisionDetectionEntry, CollisionDetectionEntry>>& collisionDetectionEntriesPairs) const
{
    std::vector<CSentriesPairTrianglesPairs> return_vector;

    for (const std::pair<CollisionDetectionEntry, CollisionDetectionEntry>& this_collisionDetectionEntriesPair : collisionDetectionEntriesPairs)
    {
        CSentriesPairTrianglesPairs this_return;
        this_return.firstEntry = this_collisionDetectionEntriesPair.first;
        this_return.secondEntry = this_collisionDetectionEntriesPair.second;

        OBBtreesCollisionHotloop(std::make_pair(reinterpret_cast<const OBBtree*>(this_collisionDetectionEntriesPair.first.OBBtree_ptr),
                                                reinterpret_cast<const OBBtree*>(this_collisionDetectionEntriesPair.second.OBBtree_ptr)),
                                 std::make_pair(this_collisionDetectionEntriesPair.first.currentGlobalMatrix,
                                                this_collisionDetectionEntriesPair.second.currentGlobalMatrix),
                                 this_return.trianglesPairs);

        if (this_return.trianglesPairs.size())
        {
            return_vector.emplace_back(std::move(this_return));
        }
    }

    return return_vector;
}

void OBBtreesCollision::OBBtreesCollisionHotloop(const std::pair<const OBBtree*, const OBBtree*> OBBtrees_ptrs_pair,
                                                 const std::pair<const glm::mat4x4, const glm::mat4x4>& matrices_pair,
                                                 std::vector<std::pair<Triangles, Triangles>>& trianglesPairs) const
{
    Cuboid first_cuboid = matrices_pair.first * OBBtrees_ptrs_pair.first->GetOBB();
    Cuboid second_cuboid = matrices_pair.second * OBBtrees_ptrs_pair.second->GetOBB();

    if (Cuboid::IntersectCuboidsBoolean(first_cuboid, second_cuboid))
    {
        if (OBBtrees_ptrs_pair.first->IsLeaf() && OBBtrees_ptrs_pair.second->IsLeaf())
        {
            trianglesPairs.emplace_back(std::make_pair(OBBtrees_ptrs_pair.first->GetTriangles(),
                                                       OBBtrees_ptrs_pair.second->GetTriangles()));
        }
        else if (!OBBtrees_ptrs_pair.first->IsLeaf() && !OBBtrees_ptrs_pair.second->IsLeaf())
        {
            if (first_cuboid.GetSurface() > second_cuboid.GetSurface())
            {
                OBBtreesCollisionHotloop(std::make_pair(OBBtrees_ptrs_pair.first->GetLeftChildPtr(),
                                                        OBBtrees_ptrs_pair.second),
                                         matrices_pair,
                                         trianglesPairs);
                OBBtreesCollisionHotloop(std::make_pair(OBBtrees_ptrs_pair.first->GetRightChildPtr(),
                                                        OBBtrees_ptrs_pair.second),
                                         matrices_pair,
                                         trianglesPairs);
            }
            else
            {
                OBBtreesCollisionHotloop(std::make_pair(OBBtrees_ptrs_pair.first,
                                                        OBBtrees_ptrs_pair.second->GetLeftChildPtr()),
                                         matrices_pair,
                                         trianglesPairs);
                OBBtreesCollisionHotloop(std::make_pair(OBBtrees_ptrs_pair.first,
                                                        OBBtrees_ptrs_pair.second->GetRightChildPtr()),
                                         matrices_pair,
                                         trianglesPairs);
            }
        }
        else if (OBBtrees_ptrs_pair.first->IsLeaf() && !OBBtrees_ptrs_pair.second->IsLeaf())
        {
            OBBtreesCollisionHotloop(std::make_pair(OBBtrees_ptrs_pair.first,
                                                    OBBtrees_ptrs_pair.second->GetLeftChildPtr()),
                                     matrices_pair,
                                     trianglesPairs);
            OBBtreesCollisionHotloop(std::make_pair(OBBtrees_ptrs_pair.first,
                                                    OBBtrees_ptrs_pair.second->GetRightChildPtr()),
                                     matrices_pair,
                                     trianglesPairs);
        }
        else
        {
            OBBtreesCollisionHotloop(std::make_pair(OBBtrees_ptrs_pair.first->GetLeftChildPtr(),
                                                    OBBtrees_ptrs_pair.second),
                                     matrices_pair,
                                     trianglesPairs);
            OBBtreesCollisionHotloop(std::make_pair(OBBtrees_ptrs_pair.first->GetRightChildPtr(),
                                                    OBBtrees_ptrs_pair.second),
                                     matrices_pair,
                                     trianglesPairs);
        }
    }
}


