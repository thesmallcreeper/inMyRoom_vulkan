cmake_minimum_required(VERSION 3.12)

project(inMyRoom_vulkan)

add_subdirectory(../Anvil "${CMAKE_CURRENT_BINARY_DIR}/anvil")
add_subdirectory(../Compressonator/Compressonator/CMP_Framework "${CMAKE_CURRENT_BINARY_DIR}/compressonator")

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
    set(OPT_FLAGS "-O3 -ffast-math -march=corei7-avx")
    set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE} ${COMMON_FLAGS} ${OPT_FLAGS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${COMMON_FLAGS}  ${OPT_FLAGS}")
endif()

add_compile_definitions(TINYGLTF_NO_INCLUDE_STB_IMAGE)
add_compile_definitions(TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE)
add_compile_definitions(TINYGLTF_NO_EXTERNAL_IMAGE)
add_compile_definitions(TINYGLTF_NO_STB_IMAGE)
add_compile_definitions(TINYGLTF_NO_STB_IMAGE_WRITE)

find_package(Threads REQUIRED)

include_directories(
        ../Anvil/include                                                   #Include Anvil
        ../Compressonator/Compressonator                                   #Include Compressonator
        ../tinygltf                                                        #Include tinyglTF
        ../Configuru                                                       #Include configuru
        ../glm                                                             #Include glm
        ../DiTO                                                            #include DiTO
        ${inMyRoom_vulkan_SOURCE_DIR}/include)

target_include_directories(Anvil PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/anvil/include")

SET(SRC #external .h
        "${inMyRoom_vulkan_SOURCE_DIR}/../DiTO/dito.h"
   
        #external .cpp
        "${inMyRoom_vulkan_SOURCE_DIR}/../DiTO/dito.cpp"
        
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
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralCompEntities/ModelCollisionCompEntity.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralCompEntities/ModelDrawCompEntity.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralCompEntities/NodeDataCompEntity.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralCompEntities/SkinCompEntity.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/AnimationActorComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/AnimationComposerComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/CameraComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/CameraDefaultInputComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/EarlyNodeGlobalMatrixComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/LateNodeGlobalMatrixComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/ModelCollisionComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/ModelDrawComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/NodeDataComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/SkinComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Geometry/FrustumCulling.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Geometry/OBB.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Geometry/OBBtree.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Geometry/Paralgram.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Geometry/Plane.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Geometry/Ray.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Geometry/Triangle.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Geometry/ViewportFrustum.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Drawer.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Graphics.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/MipmapsGenerator.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/PipelinesFactory.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/ShadersSetsFamiliesCache.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/VulkanInit.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Meshes/AnimationsDataOfNodes.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Meshes/ImagesAboutOfTextures.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Meshes/MaterialsOfPrimitives.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Meshes/MeshesOfNodes.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Meshes/PrimitivesOfMeshes.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Meshes/SkinsOfMeshes.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics/Meshes/TexturesOfMaterials.h"

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
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralCompEntities/ModelCollisionCompEntity.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralCompEntities/ModelDrawCompEntity.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralCompEntities/NodeDataCompEntity.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralCompEntities/SkinCompEntity.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/AnimationActorComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/AnimationComposerComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/CameraComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/CameraDefaultInputComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/EarlyNodeGlobalMatrixComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/LateNodeGlobalMatrixComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/ModelCollisionComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/ModelDrawComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/NodeDataComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/SkinComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Geometry/FrustumCulling.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Geometry/OBB.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Geometry/OBBtree.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Geometry/Paralgram.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Geometry/Plane.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Geometry/Ray.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Geometry/Triangle.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Geometry/ViewportFrustum.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Drawer.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Graphics.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/MipmapsGenerator.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/PipelinesFactory.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/ShadersSetsFamiliesCache.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/VulkanInit.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Meshes/AnimationsDataOfNodes.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Meshes/ImagesAboutOfTextures.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Meshes/MaterialsOfPrimitives.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Meshes/MeshesOfNodes.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Meshes/PrimitivesOfMeshes.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Meshes/SkinsOfMeshes.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics/Meshes/TexturesOfMaterials.cpp"
    )

add_executable(inMyRoom_vulkan ${SRC})

if (MSVC)
    set_target_properties(inMyRoom_vulkan PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${inMyRoom_vulkan_SOURCE_DIR}")
endif()

add_dependencies(inMyRoom_vulkan Anvil)
add_dependencies(inMyRoom_vulkan CMP_Framework)

if (WIN32)
    target_link_libraries(inMyRoom_vulkan Anvil)
    target_link_libraries(inMyRoom_vulkan CMP_Framework)
else ()
    target_link_libraries(inMyRoom_vulkan Anvil dl)
    target_link_libraries(inMyRoom_vulkan CMP_Framework dl)
endif ()

if (UNIX)
    target_link_libraries(inMyRoom_vulkan stdc++fs)
endif ()
