#include "NodesOfScene.h"

#include <stack>
#include <cassert>

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"


NodesOfScene::NodesOfScene(const tinygltf::Model& in_model, const tinygltf::Scene& in_scene,
                           MeshesOfNodes* in_meshesOfNodes_ptr, Anvil::BaseDevice* const in_device_ptr)
    :meshesOfNodes_ptr(in_meshesOfNodes_ptr),
     device_ptr(in_device_ptr)
{
    NodeRef root_emtry_node;
    uint32_t emtry_node_childrenCount = 0;      // Useful hack

    nodes.emplace_back(root_emtry_node);

    for (size_t i : in_scene.nodes)
    {
        const tinygltf::Node& this_node = in_model.nodes[i];
        const glm::mat4 parent_global_trs_matrix = CreateTRSmatrix(this_node);

        NodeRef root_nodeRef;

        if (this_node.mesh > -1)
        {
            root_nodeRef.isMesh = true;
            root_nodeRef.objectID = static_cast<uint32_t>(meshesById_TRS.size());
            root_nodeRef.meshIndex = this_node.mesh;

            emtry_node_childrenCount++;

            nodes.emplace_back(root_nodeRef);
            meshesById_TRS.emplace_back(parent_global_trs_matrix);
        }
        else if (!this_node.children.empty())
        {
            root_nodeRef.isMesh = false;
            root_nodeRef.childrenCount = static_cast<uint32_t>(this_node.children.size());

            emtry_node_childrenCount++;

            const size_t node_index = nodes.size();
            nodes.emplace_back(root_nodeRef);

            size_t size_of_nodes_vector_before = nodes.size();
            for (int this_child : this_node.children)
            {
                AddNode(in_model, in_model.nodes[this_child], parent_global_trs_matrix,
                        std::ref(meshesById_TRS));
            }

            size_t size_of_nodes_vector_after = nodes.size();
            nodes[node_index].nodeAndChildrenSize = size_of_nodes_vector_after - size_of_nodes_vector_before;
        }
    }

    nodes[0].childrenCount = emtry_node_childrenCount;
}

void NodesOfScene::AddNode(const tinygltf::Model& in_model, const tinygltf::Node& in_gltf_node,
                               const glm::mat4 parentTRSmatrix, std::vector<glm::mat4>& meshesByIdTRS)
{
    glm::mat4 node_local_trs_matrix = CreateTRSmatrix(in_gltf_node);
    glm::mat4 this_global_trs_matrix = parentTRSmatrix * node_local_trs_matrix;

    NodeRef this_nodeRef;

    if (in_gltf_node.mesh > -1)
    {
        this_nodeRef.isMesh = true;
        this_nodeRef.objectID = static_cast<uint32_t>(meshesByIdTRS.size());
        this_nodeRef.meshIndex = in_gltf_node.mesh;

        nodes.emplace_back(this_nodeRef);
        meshesByIdTRS.emplace_back(this_global_trs_matrix);
    }
    else if (!in_gltf_node.children.empty())
    {
        this_nodeRef.isMesh = false;
        this_nodeRef.childrenCount = static_cast<uint32_t>(in_gltf_node.children.size());

        size_t node_index = nodes.size();
        nodes.emplace_back(this_nodeRef);

        size_t size_of_nodes_vector_before = nodes.size();
        for (size_t i = 0; i < in_gltf_node.children.size(); i++)
        {
            AddNode(in_model, in_model.nodes[in_gltf_node.children[i]], this_global_trs_matrix,
                    std::ref(meshesByIdTRS));
        }

        size_t size_of_nodes_vector_after = nodes.size();
        nodes[node_index].nodeAndChildrenSize = size_of_nodes_vector_after - size_of_nodes_vector_before;
    }
}

std::vector<DrawRequest> NodesOfScene::DrawUsingFrustumCull(const std::array<Plane, 6> in_viewport_planes)
{
    std::vector<DrawRequest> draw_requests;

    FrustumCulling frustum_culling;        // Set up frustum culling
    frustum_culling.setFrustumPlanes(in_viewport_planes);

    size_t index = 0;

    std::stack<NodeRecursive> depth_nodes_stack;
   
    // root
    NodeRecursive root_node_state;
    root_node_state.childrenCount = nodes[index].childrenCount;
    root_node_state.childrenSoFar = 0;

    depth_nodes_stack.push(root_node_state);
    index++;

    while (depth_nodes_stack.size())
    {
        if (depth_nodes_stack.top().childrenSoFar != depth_nodes_stack.top().childrenCount)
        {
            depth_nodes_stack.top().childrenSoFar++;

            NodeRef& this_node = nodes[index];

            if (this_node.isMesh)
            {
                const MeshRange& this_mesh_range = meshesOfNodes_ptr->meshes[this_node.meshIndex];
                const glm::mat4x4& this_TRS_matrix = meshesById_TRS[this_node.objectID];

                const OBB& this_OBB = this_mesh_range.boundBox;
                Cuboid this_cuboid = this_TRS_matrix * this_OBB;

                if (frustum_culling.isCuboidInsideFrustum(this_cuboid)) // per mesh-object sphere culling
                    for (size_t primitive_index = this_mesh_range.primitiveFirstOffset; primitive_index < this_mesh_range.primitiveFirstOffset + this_mesh_range.primitiveRangeSize; primitive_index++)
                    {
                        DrawRequest this_draw_request;
                        this_draw_request.primitiveIndex = primitive_index;
                        this_draw_request.objectID = this_node.objectID;
                        this_draw_request.TRSmatrix = this_TRS_matrix;
                        draw_requests.emplace_back(this_draw_request);
                    }

                index++;
            }
            else
            {
                NodeRecursive entering_node_state;
                entering_node_state.childrenCount = this_node.childrenCount;
                entering_node_state.childrenSoFar = 0;

                depth_nodes_stack.push(entering_node_state);

                index++;
            }
        }
        else
            depth_nodes_stack.pop();
    }
    return draw_requests;
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
            return_matrix = translate(glm::mat4(1.0f), glm::vec3( in_node.translation[0], -in_node.translation[1], -in_node.translation[2])) * return_matrix;
        }

        return return_matrix;
    }
}

