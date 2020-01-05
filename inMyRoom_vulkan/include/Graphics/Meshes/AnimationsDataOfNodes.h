#pragma once

#include "ECS/ECStypes.h"

class AnimationsDataOfNodes
{
public:
    AnimationsDataOfNodes();
    ~AnimationsDataOfNodes();

    size_t RegistAnimationsDataAndGetIndex();
    void AddAnimationData(size_t index, const AnimationData& in_animationData);

    const AnimationData& GetAnimationDataRef(size_t index) const;

private:
    std::vector<AnimationData> animationsData;
};