#include "Geometry/OBB.h"

#include "dito.h"

OBB OBB::CreateOBB(const std::vector<glm::vec3>& in_points)
{
    DiTO::OBB<float> dito_OBB;
    const DiTO::Vector<float>* dito_points = reinterpret_cast<const DiTO::Vector<float>*>(in_points.data());

    DiTO_14(const_cast<DiTO::Vector<float>*>(dito_points), static_cast<int>(in_points.size()), dito_OBB);
    // const_casting was used in order to avoid editing dito.h/.cpp which indeed, according to its docs, does not modifies dito_points but does not make use of "const"

    OBB return_OBB;
    return_OBB.center = glm::vec4(dito_OBB.mid.x, dito_OBB.mid.y, dito_OBB.mid.z, 1);
    return_OBB.sideDirections.u = glm::vec4(dito_OBB.v0.x, dito_OBB.v0.y, dito_OBB.v0.z, 0);
    return_OBB.sideDirections.v = glm::vec4(dito_OBB.v1.x, dito_OBB.v1.y, dito_OBB.v1.z, 0);
    return_OBB.sideDirections.w = glm::vec4(dito_OBB.v2.x, dito_OBB.v2.y, dito_OBB.v2.z, 0);
    return_OBB.halfLengths = glm::vec3(dito_OBB.ext.x, dito_OBB.ext.y, dito_OBB.ext.z);

    return return_OBB;
}