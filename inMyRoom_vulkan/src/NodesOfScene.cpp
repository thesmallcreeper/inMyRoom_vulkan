#include "NodesOfScene.h"

#include <stack>
#include <cassert>


NodesOfScene::NodesOfScene(const tinygltf::Model& in_model, const tinygltf::Scene& in_scene,
                           MeshesOfNodes* in_meshesOfNodes_ptr, Anvil::BaseDevice* const in_device_ptr)
    :meshesOfNodes_ptr(in_meshesOfNodes_ptr),
     device_ptr(in_device_ptr)
{
    NodeInfo world_space_node;
    world_space_node.isMesh = false;
    world_space_node.localTRSmatrix = glm::mat4x4(1.f);

    uint32_t entry_node_childrenCount = 0;

    nodesInfos.emplace_back(world_space_node);

    for (size_t i : in_scene.nodes)
    {
        const tinygltf::Node& this_node = in_model.nodes[i];
        const glm::mat4 local_trs_matrix = CreateTRSmatrix(this_node);

        NodeInfo new_nodeInfo;
        new_nodeInfo.localTRSmatrix = local_trs_matrix;

        if (this_node.mesh > -1)
        {
            new_nodeInfo.isMesh = true;
            new_nodeInfo.objectID = nextObjectsID++;
            new_nodeInfo.meshIndex = this_node.mesh + meshesOfNodes_ptr->GetMeshIndexOffsetOfModel(in_model);

            entry_node_childrenCount++;

            nodesInfos.emplace_back(new_nodeInfo);
        }
        else if (!this_node.children.empty())
        {
            new_nodeInfo.isMesh = false;
            new_nodeInfo.childrenCount = static_cast<uint32_t>(this_node.children.size());

            entry_node_childrenCount++;

            nodesInfos.emplace_back(new_nodeInfo);
            NodeInfo& this_node_ref_in_vector = *nodesInfos.rbegin();

            size_t size_of_nodes_vector_before = nodesInfos.size();
            for (int this_child : this_node.children)
            {
                AddNode(in_model, in_model.nodes[this_child]);
            }
            size_t size_of_nodes_vector_after = nodesInfos.size();

            this_node_ref_in_vector.nodeAndChildrenSize = static_cast<uint32_t>(size_of_nodes_vector_after - size_of_nodes_vector_before);
        }
    }

    nodesInfos[0].childrenCount = entry_node_childrenCount;
}

void NodesOfScene::AddNode(const tinygltf::Model& in_model, 
                           const tinygltf::Node& in_gltf_node)
{
    NodeInfo new_nodeInfo;

    if (in_gltf_node.mesh > -1)
    {
        new_nodeInfo.isMesh = true;
        new_nodeInfo.objectID = nextObjectsID++;
        new_nodeInfo.meshIndex = in_gltf_node.mesh + meshesOfNodes_ptr->GetMeshIndexOffsetOfModel(in_model);

        nodesInfos.emplace_back(new_nodeInfo);
    }
    else if (!in_gltf_node.children.empty())
    {
        new_nodeInfo.isMesh = false;
        new_nodeInfo.childrenCount = static_cast<uint32_t>(in_gltf_node.children.size());
        new_nodeInfo.localTRSmatrix = CreateTRSmatrix(in_gltf_node);

        nodesInfos.emplace_back(new_nodeInfo);
        NodeInfo& this_node_ref_in_vector = *nodesInfos.rbegin();

        size_t size_of_nodes_vector_before = nodesInfos.size();
        for (size_t i = 0; i < in_gltf_node.children.size(); i++)
        {
            AddNode(in_model, in_model.nodes[in_gltf_node.children[i]]);
        }
        size_t size_of_nodes_vector_after = nodesInfos.size();

        this_node_ref_in_vector.nodeAndChildrenSize = static_cast<uint32_t>(size_of_nodes_vector_after - size_of_nodes_vector_before);
    }
}

std::vector<DrawRequest> NodesOfScene::DrawUsingFrustumCull(const std::array<Plane, 6> in_viewport_planes)
{
    std::vector<DrawRequest> draw_requests;

    FrustumCulling frustum_culling;        // Set up frustum culling
    frustum_culling.SetFrustumPlanes(in_viewport_planes);

    size_t node_index = 0;

    std::stack<NodeRecursive> depth_nodes_stack;
   
    // root
    NodeRecursive root_node_state;
    root_node_state.childrenCount = nodesInfos[node_index++].childrenCount;
    root_node_state.childrenSoFar = 0;
    root_node_state.globalTRSmaxtix = glm::mat4x4(1.f);

    depth_nodes_stack.push(root_node_state);

    while (depth_nodes_stack.size())
    {
        if (depth_nodes_stack.top().childrenSoFar != depth_nodes_stack.top().childrenCount)
        {
            depth_nodes_stack.top().childrenSoFar++;

            const NodeInfo& this_node = nodesInfos[node_index++];

            if (this_node.shouldRender)
            {
                const glm::mat4x4& global_TRS_matrix = depth_nodes_stack.top().globalTRSmaxtix * this_node.localTRSmatrix;

                if (this_node.isMesh)
                {
                    const MeshInfo this_mesh_range = meshesOfNodes_ptr->GetMesh(this_node.meshIndex);

                    const OBB& this_OBB = this_mesh_range.boundBox;
                    Cuboid this_cuboid = global_TRS_matrix * this_OBB;

                    if (frustum_culling.IsCuboidInsideFrustum(this_cuboid)) // per mesh-object OBB culling
                        for (size_t primitive_index = this_mesh_range.primitiveFirstOffset; primitive_index < this_mesh_range.primitiveFirstOffset + this_mesh_range.primitiveRangeSize; primitive_index++)
                        {
                            DrawRequest this_draw_request;
                            this_draw_request.primitiveIndex = primitive_index;
                            this_draw_request.objectID = this_node.objectID;
                            this_draw_request.TRSmatrix = global_TRS_matrix;
                            draw_requests.emplace_back(this_draw_request);
                        }
                }
                else
                {
                    NodeRecursive entering_node_state;
                    entering_node_state.childrenCount = this_node.childrenCount;
                    entering_node_state.childrenSoFar = 0;
                    entering_node_state.globalTRSmaxtix = global_TRS_matrix;

                    depth_nodes_stack.push(entering_node_state);
                }
            }
            else
            {
                node_index += this_node.nodeAndChildrenSize;
            }
        }
        else
        {
            depth_nodes_stack.pop();
        }
    }
    return std::move(draw_requests);
}

glm::mat4 NodesOfScene::CreateTRSmatrix(const tinygltf::Node& in_node) const
{
    if (in_node.matrix.size() == 16)
    {
        return  glm::mat4( 1.f, 0.f, 0.f, 0.f,
                           0.f,-1.f, 0.f, 0.f,
                           0.f, 0.f,-1.f, 0.f,
                           0.f, 0.f, 0.f, 1.f)
              * glm::mat4(in_node.matrix[0] , in_node.matrix[1] , in_node.matrix[2] , in_node.matrix[3] ,
                          in_node.matrix[4] , in_node.matrix[5] , in_node.matrix[6] , in_node.matrix[7] ,
                          in_node.matrix[8] , in_node.matrix[9] , in_node.matrix[10], in_node.matrix[11],
                          in_node.matrix[12], in_node.matrix[13], in_node.matrix[14], in_node.matrix[15])
              * glm::mat4(1.f, 0.f, 0.f, 0.f,
                          0.f,-1.f, 0.f, 0.f,
                          0.f, 0.f,-1.f, 0.f,
                          0.f, 0.f, 0.f, 1.f);
    }
    else
    {
        glm::mat4 return_matrix = glm::mat4(1.0f);
        if (in_node.scale.size() == 3)
        {
            return_matrix = glm::scale(glm::mat4(1.0f), glm::vec3(in_node.scale[0], in_node.scale[1], in_node.scale[2])) * return_matrix;
        }
        if (in_node.rotation.size() == 4)
        {
            const glm::qua<float> rotation_qua(static_cast<float>(in_node.rotation[3]), static_cast<float>(in_node.rotation[0]), static_cast<float>(-in_node.rotation[1]), static_cast<float>(-in_node.rotation[2]));

            return_matrix = glm::toMat4<float, glm::packed_highp>(rotation_qua) * return_matrix;
        }
        if (in_node.translation.size() == 3)
        {
            return_matrix = glm::translate(glm::mat4(1.0f), glm::vec3( in_node.translation[0], -in_node.translation[1], -in_node.translation[2])) * return_matrix;
        }

        return return_matrix;
    }
}

