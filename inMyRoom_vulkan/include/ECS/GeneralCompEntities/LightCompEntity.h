#pragma once

#include "ECS/CompEntityBaseWrappedClass.h"

class LightComp;

#include "ECS/GeneralCompEntities/LateNodeGlobalMatrixCompEntity.h"

class LightCompEntity :
        public CompEntityBaseWrappedClass<LightComp>
{
#ifndef GAME_DLL
public:
    LightCompEntity(Entity this_entity);

    static LightCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - LightCompEntity
        "LightType",                     lightType                     = string
        "Radius",                        radius                        = float
        "Length",                        length                        = float
        "Range",                         range                         = float
        "Luminance",                     luminance                     = glm::vec4
    */
    static LightCompEntity CreateComponentEntityByMap(Entity in_entity, std::string entity_name, const CompEntityInitMap& in_map);

    void Update(class Lights* lights_ptr);
    void ToBeRemovedCallBack(class Lights* lights_ptr);

    void AddLightInfo(const LateNodeGlobalMatrixComp* nodeGlobalMatrix_ptr,
                      const glm::mat4& viewport_matrix,
                      std::vector<ModelMatrices>& model_matrices,
                      std::vector<LightInfo>& light_infos);

#endif
    void SetLuminance(const glm::vec3& in_luminance) {luminance = in_luminance;}
    void SetLightOn(bool state) {lightOn = state;}

    bool ShouldDraw() const;

private:
    glm::mat4 GetLightMatrix(const glm::mat4& matrix) const;

public: // data
    LightType lightType = LightType::Sphere;
    float radius = 1.f;
    float length = 1.f;
    glm::vec3 luminance = glm::vec3(1.f, 1.f, 1.f);
    float range = 10.f;

    bool lightOn = true;

    size_t matricesOffset = -1;

    size_t lightIndex = -1;
    bool toBeRemoved = false;
};

#ifdef GAME_DLL
class LightComp
    :public ComponentBaseWrappedClass<LightCompEntity, static_cast<componentID>(componentIDenum::Light), "Light", sparse_set> {};
#else
#include "ECS/GeneralComponents/LightComp.h"
#endif
