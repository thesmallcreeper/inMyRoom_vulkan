cmake_minimum_required(VERSION 3.12)

project(inMyRoom_vulkan)

add_subdirectory(../glfw "${CMAKE_CURRENT_BINARY_DIR}/glfw")

set(NRD_STATIC_LIBRARY ON)
set(NRD_SHADER_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/shaders/NRD/binary")
set(NRD_USE_OCT_NORMAL_ENCODING "1")
add_subdirectory(../RayTracingDenoiser "${CMAKE_CURRENT_BINARY_DIR}/RayTracingDenoiser")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(THREADS_PREFER_PTHREAD_FLAG ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${inMyRoom_vulkan_SOURCE_DIR})

if (MSVC)
    add_definitions(/arch:AVX)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(NOMINMAX)
    set(relFlags "/O2 /fp:fast /fp:except- /MP")
    set(CMAKE_C_FLAGS_RELEASE     "${CMAKE_C_FLAGS_RELEASE} ${relFlags}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${relFlags}")
else ()
    set(COMMON_FLAGS "-m64")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${COMMON_FLAGS} -D_DEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${COMMON_FLAGS} -D_DEBUG")
    set(OPT_FLAGS "-O3 -ffast-math -march=corei7-avx -DNDEBUG")
    set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE} ${COMMON_FLAGS} ${OPT_FLAGS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${COMMON_FLAGS}  ${OPT_FLAGS}")
endif()

#add_compile_definitions(CPU_WAIT_GPU_SERIALIZED)
add_compile_definitions(ENABLE_ASYNC_COMPUTE)

add_compile_definitions(TINYGLTF_NOEXCEPTION)

add_compile_definitions(VK_NO_PROTOTYPES)
add_compile_definitions(VULKAN_HPP_NO_NODISCARD_WARNINGS=1)

add_compile_definitions(VULKAN_HPP_NO_EXCEPTIONS=1)

add_compile_definitions(VMA_STATIC_VULKAN_FUNCTIONS=0)
add_compile_definitions(VMA_DYNAMIC_VULKAN_FUNCTIONS=1)

add_compile_definitions(GLFW_INCLUDE_NONE)

add_compile_definitions(ENABLE_CPP_INTERFACE)

find_package(Threads REQUIRED)

include_directories(
        ${inMyRoom_vulkan_SOURCE_DIR}/include
        ${inMyRoom_vulkan_SOURCE_DIR}/shaders
        $ENV{VULKAN_SDK}/include                                           #Include Vulkan Sdk
        ../VulkanMemoryAllocator-Hpp                                       #Include VMA
        ../glfw/include                                                    #Include glfw
        ../tinygltf                                                        #Include tinyglTF
        ../Configuru                                                       #Include configuru
        ../glm                                                             #Include glm
        ../eig3                                                            #Include eig3
        ../RayTracingDenoiser/include                                      #Include NRD
)

SET(SRC #eig3    .h
        "${inMyRoom_vulkan_SOURCE_DIR}/../eig3/eig3.h"

        #eig3    .cpp
        "${inMyRoom_vulkan_SOURCE_DIR}/../eig3/eig3.cpp"

        #headers .h
        "${inMyRoom_vulkan_SOURCE_DIR}/include/const_maps.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/dense_set.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Engine.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ExportedFunctionsConstructor.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/FixedString.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/GameDLLimporter.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/GameImporter.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/game_dll.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/glTFenum.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/hash_combine.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/InputManager.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/sparse_set.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/WindowWithAsyncInput.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/CollisionDetection/CollisionDetection.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/CollisionDetection/CreateUncollideRays.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/CollisionDetection/OBBtreesCollision.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/CollisionDetection/ShootUncollideRays.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/CollisionDetection/SweepAndPrune.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/CompEntityBaseClass.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/CompEntityBaseWrappedClass.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/ComponentBaseClass.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/ComponentBaseWrappedClass.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/ComponentDataClass.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/ComponentsIDsEnum.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/ConceptHelpers.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/ECStypes.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/ECSwrapper.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/EntitiesHandler.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/ExportedFunctions.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/TemplateHelpers.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralCompEntities/AnimationActorCompEntity.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralCompEntities/AnimationComposerCompEntity.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralCompEntities/CameraCompEntity.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralCompEntities/CameraDefaultInputCompEntity.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralCompEntities/EarlyNodeGlobalMatrixCompEntity.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralCompEntities/LateNodeGlobalMatrixCompEntity.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralCompEntities/LightCompEntity.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralCompEntities/ModelCollisionCompEntity.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralCompEntities/ModelDrawCompEntity.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralCompEntities/NodeDataCompEntity.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralCompEntities/DynamicMeshCompEntity.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/AnimationActorComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/AnimationComposerComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/CameraComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/CameraDefaultInputComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/EarlyNodeGlobalMatrixComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/LateNodeGlobalMatrixComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/LightComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/ModelCollisionComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/ModelDrawComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/NodeDataComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/DynamicMeshComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Geometry/Cylinder.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Geometry/FrustumCulling.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Geometry/OBB.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Geometry/OBBtree.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Geometry/Paralgram.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Geometry/Plane.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Geometry/Ray.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Geometry/Sphere.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Geometry/Triangle.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Geometry/ViewportFrustum.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/DynamicMeshes.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Graphics.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/PipelinesFactory.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/ShadersSetsFamiliesCache.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/VulkanInit.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/HelperUtils.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/ImageData.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Lights.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/RendererBase.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/TLASbuilder.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Exposure.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Meshes/AnimationsDataOfNodes.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Meshes/MaterialsOfPrimitives.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Meshes/MeshesOfNodes.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Meshes/PrimitivesOfMeshes.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Meshes/SkinsOfMeshes.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Meshes/TexturesOfMaterials.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Renderers/OfflineRenderer.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Renderers/RealtimeRenderer.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Textures/TextureImage.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Textures/ColorImage.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Textures/NormalImage.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Textures/MetallicRoughnessImage.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/NRDintegration.h"

        #source .cpp
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Engine.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ExportedFunctionsConstructor.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/GameDLLimporter.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/GameImporter.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/implementations.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/InputManager.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/main.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/WindowWithAsyncInput.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/CollisionDetection/CollisionDetection.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/CollisionDetection/CreateUncollideRays.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/CollisionDetection/OBBtreesCollision.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/CollisionDetection/ShootUncollideRays.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/CollisionDetection/SweepAndPrune.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/ComponentBaseClass.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/ECSwrapper.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/EntitiesHandler.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralCompEntities/AnimationActorCompEntity.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralCompEntities/AnimationComposerCompEntity.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralCompEntities/CameraCompEntity.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralCompEntities/CameraDefaultInputCompEntity.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralCompEntities/EarlyNodeGlobalMatrixCompEntity.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralCompEntities/LateNodeGlobalMatrixCompEntity.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralCompEntities/LightCompEntity.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralCompEntities/ModelCollisionCompEntity.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralCompEntities/ModelDrawCompEntity.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralCompEntities/NodeDataCompEntity.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralCompEntities/DynamicMeshCompEntity.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/AnimationActorComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/AnimationComposerComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/CameraComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/CameraDefaultInputComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/EarlyNodeGlobalMatrixComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/LateNodeGlobalMatrixComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/LightComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/ModelCollisionComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/ModelDrawComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/NodeDataComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/DynamicMeshComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Geometry/Cylinder.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Geometry/FrustumCulling.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Geometry/OBB.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Geometry/OBBtree.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Geometry/Paralgram.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Geometry/Plane.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Geometry/Ray.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Geometry/Sphere.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Geometry/Triangle.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Geometry/ViewportFrustum.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/DynamicMeshes.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Graphics.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/PipelinesFactory.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/ShadersSetsFamiliesCache.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/VulkanInit.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/HelperUtils.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/ImageData.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Lights.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/RendererBase.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/TLASbuilder.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Exposure.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Meshes/AnimationsDataOfNodes.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Meshes/MaterialsOfPrimitives.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Meshes/MeshesOfNodes.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Meshes/PrimitivesOfMeshes.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Meshes/SkinsOfMeshes.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Meshes/TexturesOfMaterials.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Renderers/OfflineRenderer.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Renderers/RealtimeRenderer.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Textures/TextureImage.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Textures/ColorImage.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Textures/NormalImage.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Textures/MetallicRoughnessImage.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/NRDintegration.cpp"
        )

add_executable(inMyRoom_vulkan ${SRC})

if (MSVC)
    set_target_properties(inMyRoom_vulkan PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${inMyRoom_vulkan_SOURCE_DIR}")
endif()

add_dependencies(inMyRoom_vulkan glfw)

if (WIN32)
    target_link_libraries(inMyRoom_vulkan glfw)
    target_link_libraries(inMyRoom_vulkan NRD)
    target_link_libraries(inMyRoom_vulkan debug $ENV{VULKAN_SDK}/lib/shaderc_combinedd.lib
                                          optimized $ENV{VULKAN_SDK}/lib/shaderc_combined.lib)
else ()
    target_link_libraries(inMyRoom_vulkan dl)
    target_link_libraries(inMyRoom_vulkan glfw)
    target_link_libraries(inMyRoom_vulkan NRD)
    target_link_libraries(inMyRoom_vulkan $ENV{VULKAN_SDK}/lib/libshaderc_combined.a)
endif ()
