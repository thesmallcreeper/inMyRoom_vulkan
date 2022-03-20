cmake_minimum_required(VERSION 3.12)

project(game_dll)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(THREADS_PREFER_PTHREAD_FLAG ON)

if (MSVC)
    add_definitions(/arch:AVX)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(NOMINMAX)
    set(relFlags "/O2 /fp:fast /fp:except- /MP")
    set(CMAKE_C_FLAGS_RELEASE     "${CMAKE_C_FLAGS_RELEASE} ${relFlags}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${relFlags}")
else ()
    set(COMMON_FLAGS "-m64 -fvisibility=hidden")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${COMMON_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${COMMON_FLAGS}")
    set(OPT_FLAGS "-O3 -ffast-math -march=corei7-avx")
    set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE} ${COMMON_FLAGS} ${OPT_FLAGS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${COMMON_FLAGS}  ${OPT_FLAGS}")
endif()

add_compile_definitions(GAME_DLL)
add_compile_definitions(ENABLE_CPP_INTERFACE)
find_package(Threads REQUIRED)

include_directories(
    ../../../../glm                         # Include glm
    ../../../../inMyRoom_vulkan/include     # Include engine's header files
    ../../../../inMyRoom_vulkan/shaders     # Include engine's shaders files
    ${game_dll_SOURCE_DIR}/include)

SET(SRC #engine's .h
        "${game_dll_SOURCE_DIR}/../../../include/sparse_set.h" 
        "${game_dll_SOURCE_DIR}/../../../include/dense_set.h" 
        "${game_dll_SOURCE_DIR}/../../../include/ECS/CompEntityBaseClass.h" 
        "${game_dll_SOURCE_DIR}/../../../include/ECS/CompEntityBaseWrappedClass.h" 
        "${game_dll_SOURCE_DIR}/../../../include/ECS/ComponentBaseClass.h" 
        "${game_dll_SOURCE_DIR}/../../../include/ECS/ComponentBaseWrappedClass.h" 
        "${game_dll_SOURCE_DIR}/../../../include/ECS/ComponentsIDsEnum.h"  
        "${game_dll_SOURCE_DIR}/../../../include/ECS/ECStypes.h" 
        "${game_dll_SOURCE_DIR}/../../../include/ECS/ECSwrapper.h" 
        "${game_dll_SOURCE_DIR}/../../../include/ECS/EntitiesHandler.h" 
        "${game_dll_SOURCE_DIR}/../../../include/ECS/ExportedFunctions.h" 
        "${game_dll_SOURCE_DIR}/../../../include/ECS/GeneralCompEntities/AnimationActorCompEntity.h" 
        "${game_dll_SOURCE_DIR}/../../../include/ECS/GeneralCompEntities/AnimationComposerCompEntity.h" 
        "${game_dll_SOURCE_DIR}/../../../include/ECS/GeneralCompEntities/CameraCompEntity.h" 
        "${game_dll_SOURCE_DIR}/../../../include/ECS/GeneralCompEntities/CameraDefaultInputCompEntity.h" 
        "${game_dll_SOURCE_DIR}/../../../include/ECS/GeneralCompEntities/EarlyNodeGlobalMatrixCompEntity.h" 
        "${game_dll_SOURCE_DIR}/../../../include/ECS/GeneralCompEntities/LateNodeGlobalMatrixCompEntity.h" 
        "${game_dll_SOURCE_DIR}/../../../include/ECS/GeneralCompEntities/ModelCollisionCompEntity.h" 
        "${game_dll_SOURCE_DIR}/../../../include/ECS/GeneralCompEntities/ModelDrawCompEntity.h" 
        "${game_dll_SOURCE_DIR}/../../../include/ECS/GeneralCompEntities/NodeDataCompEntity.h" 
        "${game_dll_SOURCE_DIR}/../../../include/ECS/GeneralCompEntities/DynamicMeshCompEntity.h"
        "${game_dll_SOURCE_DIR}/../../../include/ECS/GeneralCompEntities/LightCompEntity.h"
        "${game_dll_SOURCE_DIR}/../../../include/game_dll.h" 
        "${game_dll_SOURCE_DIR}/../../../include/Geometry/ViewportFrustum.h" 

        #engine's .cpp
        "${game_dll_SOURCE_DIR}/../../../src/ECS/ComponentBaseClass.cpp"
        "${game_dll_SOURCE_DIR}/../../../src/ECS/ECSwrapper.cpp"
        "${game_dll_SOURCE_DIR}/../../../src/ECS/EntitiesHandler.cpp"
        "${game_dll_SOURCE_DIR}/../../../src/ECS/GeneralCompEntities/AnimationActorCompEntity.cpp"
        "${game_dll_SOURCE_DIR}/../../../src/ECS/GeneralCompEntities/AnimationComposerCompEntity.cpp"
        "${game_dll_SOURCE_DIR}/../../../src/ECS/GeneralCompEntities/CameraCompEntity.cpp"
        "${game_dll_SOURCE_DIR}/../../../src/ECS/GeneralCompEntities/CameraDefaultInputCompEntity.cpp"
        "${game_dll_SOURCE_DIR}/../../../src/ECS/GeneralCompEntities/EarlyNodeGlobalMatrixCompEntity.cpp"
        "${game_dll_SOURCE_DIR}/../../../src/ECS/GeneralCompEntities/LateNodeGlobalMatrixCompEntity.cpp"
        "${game_dll_SOURCE_DIR}/../../../src/ECS/GeneralCompEntities/ModelCollisionCompEntity.cpp"
        "${game_dll_SOURCE_DIR}/../../../src/ECS/GeneralCompEntities/ModelDrawCompEntity.cpp"
        "${game_dll_SOURCE_DIR}/../../../src/ECS/GeneralCompEntities/NodeDataCompEntity.cpp"
        "${game_dll_SOURCE_DIR}/../../../src/ECS/GeneralCompEntities/DynamicMeshCompEntity.cpp"
        "${game_dll_SOURCE_DIR}/../../../src/ECS/GeneralCompEntities/LightCompEntity.cpp"
        "${game_dll_SOURCE_DIR}/../../../src/game_dll.cpp"
        "${game_dll_SOURCE_DIR}/../../../src/Geometry/ViewportFrustum.cpp"
        
        #game_dll .h
        "${game_dll_SOURCE_DIR}/include/CompEntities/SnakePlayerCompEntity.h" 
        "${game_dll_SOURCE_DIR}/include/Components/SnakePlayerComp.h" 
        "${game_dll_SOURCE_DIR}/include/GameSpecificComponentsVector.h" 
        "${game_dll_SOURCE_DIR}/include/GameSpecificComponentsIDsEnum.inj" 
        
        #game_dll .cpp
        "${game_dll_SOURCE_DIR}/src/CompEntities/SnakePlayerCompEntity.cpp"
        "${game_dll_SOURCE_DIR}/src/Components/SnakePlayerComp.cpp"
        "${game_dll_SOURCE_DIR}/src/dllmain.cpp"
    )

add_library(game_dll SHARED ${SRC})