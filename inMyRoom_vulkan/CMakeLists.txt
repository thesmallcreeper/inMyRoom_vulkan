cmake_minimum_required(VERSION 3.8)

project(inMyRoom_vulkan)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(THREADS_PREFER_PTHREAD_FLAG ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${inMyRoom_vulkan_SOURCE_DIR})

if (MSVC)
    add_definitions(/arch:AVX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(relFlags "/O2 /fp:fast /fp:except- /MP")
    set(CMAKE_C_FLAGS_RELEASE     "${CMAKE_C_FLAGS_RELEASE} ${relFlags}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${relFlags}")
else ()
    add_compile_options(-m64)
    add_definitions(-mavx -march=corei7-avx -mtune=corei7-avx)
    set(OPT_FLAGS "-O3 -ffast-math")
    set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE} ${OPT_FLAGS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${OPT_FLAGS}")
endif()

add_subdirectory(../Anvil "${CMAKE_CURRENT_BINARY_DIR}/anvil")
add_subdirectory(../Compressonator/Compressonator/Make "${CMAKE_CURRENT_BINARY_DIR}/compressonator")

find_package(Threads REQUIRED)

target_include_directories(Anvil PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/anvil/include")

if (WIN32)
    include_directories($ENV{VK_SDK_PATH}/Include
                        $ENV{VULKAN_SDK}/Include)
else()
    include_directories($ENV{VK_SDK_PATH}/x86_64/include
                        $ENV{VULKAN_SDK}/include
                        $ENV{VULKAN_SDK}/x86_64/include)
endif()

include_directories(
        ../Anvil/include                                                   #Include Anvil
        ../Compressonator/Compressonator/Header                            #Include Compressonator
        ../tinygltf                                                        #Include tinyglTF
        ../Configuru                                                       #Include configuru
        ../glm                                                             #Include glm
        ../DiTO                                                            #include DiTO
        ${inMyRoom_vulkan_SOURCE_DIR}/include)

SET(SRC #etc-.h
        "${inMyRoom_vulkan_SOURCE_DIR}/include/glTFenum.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/hash_combine.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/const_maps.h"
        #include-.h ECS
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralCompEntities/ModelDrawCompEntity.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralCompEntities/NodeGlobalMatrixCompEntity.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralCompEntities/PositionCompEntity.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/ModelDrawComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/NodeGlobalMatrixComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/GeneralComponents/PositionComp.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/ComponentBaseClass.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/ComponentRawBaseClass.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/ComponentSparseBaseClass.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/ComponentsIDsEnum.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/ECStypes.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/ECSwrapper.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/EnginesExportedFunctions.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ECS/EntitiesHandler.h"
        #include-.h geometry
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Geometry/ViewportFrustum.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Geometry/FrustumCulling.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Geometry/Plane.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Geometry/Cuboid.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Geometry/OBB.h"
        #include-.h meshes
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Meshes/MeshesOfNodes.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Meshes/MaterialsOfPrimitives.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Meshes/ImagesAboutOfTextures.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Meshes/TexturesOfMaterials.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Meshes/PrimitivesOfMeshes.h"
        #include-.h
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Engine.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/VulkanInit.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/InputManager.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/CameraBaseClass.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/NaiveCamera.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Drawer.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/WindowWithAsyncInput.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ShadersSetsFamiliesCache.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/PipelinesFactory.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/MipmapsGenerator.h"
        
        #main.cpp
        "${inMyRoom_vulkan_SOURCE_DIR}/src/main.cpp"
        #etc-.cpp
        "${inMyRoom_vulkan_SOURCE_DIR}/../DiTO/dito.cpp"
        #source-.cpp ECS
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralCompEntities/ModelDrawCompEntity.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralCompEntities/NodeGlobalMatrixCompEntity.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralCompEntities/PositionCompEntity.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/ModelDrawComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/NodeGlobalMatrixComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/GeneralComponents/PositionComp.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/ComponentBaseClass.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/ComponentRawBaseClass.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/ComponentSparseBaseClass.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/ECSwrapper.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/EnginesExportedFunctions.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ECS/EntitiesHandler.cpp"
        #source-.cpp geometry
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Geometry/ViewportFrustum.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Geometry/FrustumCulling.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Geometry/Plane.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Geometry/Cuboid.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Geometry/OBB.cpp"
        #source-.cpp meshes
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Meshes/MeshesOfNodes.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Meshes/MaterialsOfPrimitives.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Meshes/ImagesAboutOfTextures.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Meshes/TexturesOfMaterials.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Meshes/PrimitivesOfMeshes.cpp"
        #source-.cpp
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Engine.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/VulkanInit.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/InputManager.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/CameraBaseClass.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/NaiveCamera.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Drawer.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/WindowWithAsyncInput.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ShadersSetsFamiliesCache.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/PipelinesFactory.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/MipmapsGenerator.cpp"
    )

add_executable(inMyRoom_vulkan ${SRC})

if (MSVC)
    set_target_properties(inMyRoom_vulkan PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${inMyRoom_vulkan_SOURCE_DIR}")
endif()

add_dependencies(inMyRoom_vulkan Anvil)
add_dependencies(inMyRoom_vulkan Compressonator)

if (WIN32)
    target_link_libraries(inMyRoom_vulkan Anvil)
    target_link_libraries(inMyRoom_vulkan Compressonator)
else ()
    target_link_libraries(inMyRoom_vulkan Anvil dl)
    target_link_libraries(inMyRoom_vulkan Compressonator dl)
endif ()

if (UNIX)
    target_link_libraries(inMyRoom_vulkan stdc++fs)
endif ()
