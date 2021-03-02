#include "CollisionDetection/SweepAndPrune.h"

#include <vector>
#include <list>
#include <algorithm>
#include <utility>

SweepAndPrune::SweepAndPrune(glm::vec3 in_U_axis, glm::vec3 in_V_axis, glm::vec3 in_W_axis)
    :U_axis(in_U_axis),
     V_axis(in_V_axis),
     W_axis(in_W_axis)
{
}

std::vector<std::pair<CollisionDetectionEntry, CollisionDetectionEntry>> SweepAndPrune::ExecuteSweepAndPrune(const std::vector<CollisionDetectionEntry>& collisionDetectionEntries)
{
    std::vector<SweepAndPruneEntry> U_axis_entries;
    std::vector<SweepAndPruneEntry> V_axis_entries;
    std::vector<SweepAndPruneEntry> W_axis_entries;

    for (size_t index = 0; index < collisionDetectionEntries.size(); index++)
    {
        Paralgram this_paralgram = collisionDetectionEntries[index].currentGlobalMatrix * reinterpret_cast<const OBBtree*>(collisionDetectionEntries[index].OBBtree_ptr)->GetRootOBB();

        {
            SweepAndPruneEntry this_entry;
            this_entry.index = index;
            this_entry.shouldCallback = collisionDetectionEntries[index].shouldCallback;
            this_entry.minMaxProjection = this_paralgram.GetMinMaxProjectionToAxis(U_axis);
            U_axis_entries.emplace_back(this_entry);
        }
        {
            SweepAndPruneEntry this_entry;
            this_entry.index = index;
            this_entry.shouldCallback = collisionDetectionEntries[index].shouldCallback;
            this_entry.minMaxProjection = this_paralgram.GetMinMaxProjectionToAxis(V_axis);
            V_axis_entries.emplace_back(this_entry);
        }
        {
            SweepAndPruneEntry this_entry;
            this_entry.index = index;
            this_entry.shouldCallback = collisionDetectionEntries[index].shouldCallback;
            this_entry.minMaxProjection = this_paralgram.GetMinMaxProjectionToAxis(W_axis);
            W_axis_entries.emplace_back(this_entry);
        }     
    }

    std::sort(U_axis_entries.begin(), U_axis_entries.end(), 
              [](const SweepAndPruneEntry& a, const SweepAndPruneEntry& b) {return a.minMaxProjection.first < b.minMaxProjection.first; });
    std::sort(V_axis_entries.begin(), V_axis_entries.end(),
              [](const SweepAndPruneEntry& a, const SweepAndPruneEntry& b) {return a.minMaxProjection.first < b.minMaxProjection.first; });
    std::sort(W_axis_entries.begin(), W_axis_entries.end(),
              [](const SweepAndPruneEntry& a, const SweepAndPruneEntry& b) {return a.minMaxProjection.first < b.minMaxProjection.first; });


    std::unordered_map<std::pair<size_t ,size_t>, uint32_t, hash_pair, commutative_equal_pair> common_collision_umap;

    {
        std::list<SweepAndPruneEntry> active_list;

        for (SweepAndPruneEntry& this_sweepAndPruneEntry : U_axis_entries)
        {
            for (auto active_list_iter = active_list.begin(); active_list_iter != active_list.end(); )
            {

                if (active_list_iter->minMaxProjection.second < this_sweepAndPruneEntry.minMaxProjection.first)
                {
                    active_list_iter = active_list.erase(active_list_iter);
                }
                else
                {
                    if (active_list_iter->shouldCallback || this_sweepAndPruneEntry.shouldCallback)
                    {
                        common_collision_umap.emplace(std::make_pair(active_list_iter->index, this_sweepAndPruneEntry.index), 1);
                    }

                    ++active_list_iter;
                }
            }

            active_list.emplace_back(this_sweepAndPruneEntry);
        }
    }

    {
        std::list<SweepAndPruneEntry> active_list;

        for (SweepAndPruneEntry& this_sweepAndPruneEntry : V_axis_entries)
        {
            for (auto active_list_iter = active_list.begin(); active_list_iter != active_list.end(); )
            {

                if (active_list_iter->minMaxProjection.second < this_sweepAndPruneEntry.minMaxProjection.first)
                {
                    active_list_iter = active_list.erase(active_list_iter);
                }
                else
                {
                    if (active_list_iter->shouldCallback || this_sweepAndPruneEntry.shouldCallback)
                    {
                        auto search = common_collision_umap.find(std::make_pair(active_list_iter->index, this_sweepAndPruneEntry.index));
                        if (search != common_collision_umap.end())
                        {
                            search->second++;
                        }
                    }

                    ++active_list_iter;
                }
            }

            active_list.emplace_back(this_sweepAndPruneEntry);
        }
    }

    {
        std::list<SweepAndPruneEntry> active_list;

        for (SweepAndPruneEntry& this_sweepAndPruneEntry : W_axis_entries)
        {
            for (auto active_list_iter = active_list.begin(); active_list_iter != active_list.end(); )
            {

                if (active_list_iter->minMaxProjection.second < this_sweepAndPruneEntry.minMaxProjection.first)
                {
                    active_list_iter = active_list.erase(active_list_iter);
                }
                else
                {
                    if (active_list_iter->shouldCallback || this_sweepAndPruneEntry.shouldCallback)
                    {
                        auto search = common_collision_umap.find(std::make_pair(active_list_iter->index, this_sweepAndPruneEntry.index));
                        if (search != common_collision_umap.end())
                        {
                            search->second++;
                        }
                    }

                    ++active_list_iter;
                }
            }

            active_list.emplace_back(this_sweepAndPruneEntry);
        }
    }

    std::vector<std::pair<CollisionDetectionEntry, CollisionDetectionEntry>> return_vector;
    
    for (auto& it : common_collision_umap)
    {
        if (it.second == 3)
        {
            return_vector.emplace_back(collisionDetectionEntries[it.first.first], collisionDetectionEntries[it.first.second]);     
        }
    }

    return return_vector;
}
