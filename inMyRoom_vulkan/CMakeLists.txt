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
add_subdirectory(../MathGeoLib "${CMAKE_CURRENT_BINARY_DIR}/mathgeolib")

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
        ../MathGeoLib/src                                                  #Include MathGeoLib
        ../tinygltf                                                        #Include tinyglTF
        ../Configuru                                                       #Include configuru
        ../glm                                                             #Include glm
        ${inMyRoom_vulkan_SOURCE_DIR}/include)

SET(SRC #header-only
        "${inMyRoom_vulkan_SOURCE_DIR}/include/glTFenum.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/hash_combine.h"
        #includes-.h
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Engine.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/VulkanInit.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/InputManager.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ViewportFrustum.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/CameraBaseClass.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/NaiveCamera.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Drawer.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/WindowWithAsyncInput.h"
        #includes-.h of scene
        "${inMyRoom_vulkan_SOURCE_DIR}/include/NodesOfScene.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/MeshesOfNodes.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/MaterialsOfPrimitives.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/PipelinesOfPrimitives.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ShadersOfPrimitives.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/ImagesUsageOfTextures.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/TexturesOfMaterials.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/PrimitivesOfMeshes.h"
        
        #main.cpp
        "${inMyRoom_vulkan_SOURCE_DIR}/src/main.cpp"
        #source-.cpp
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Engine.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/VulkanInit.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/InputManager.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ViewportFrustum.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/CameraBaseClass.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/NaiveCamera.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Drawer.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/WindowWithAsyncInput.cpp"
        #source-.cpp of scene
        "${inMyRoom_vulkan_SOURCE_DIR}/src/NodesOfScene.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/MeshesOfNodes.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/MaterialsOfPrimitives.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/PipelinesOfPrimitives.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ShadersOfPrimitives.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/ImagesUsageOfTextures.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/TexturesOfMaterials.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/PrimitivesOfMeshes.cpp"
    )

add_executable(inMyRoom_vulkan ${SRC})

if (MSVC)
    set_target_properties(inMyRoom_vulkan PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${inMyRoom_vulkan_SOURCE_DIR}")
endif()

add_dependencies(inMyRoom_vulkan Anvil)
add_dependencies(inMyRoom_vulkan Compressonator)
add_dependencies(inMyRoom_vulkan MathGeoLib)

if (WIN32)
    target_link_libraries(inMyRoom_vulkan Anvil)
    target_link_libraries(inMyRoom_vulkan Compressonator)
    target_link_libraries(inMyRoom_vulkan MathGeoLib)
else ()
    target_link_libraries(inMyRoom_vulkan Anvil dl)
    target_link_libraries(inMyRoom_vulkan Compressonator dl)
    target_link_libraries(inMyRoom_vulkan MathGeoLib dl)
endif ()

if (UNIX)
    target_link_libraries(inMyRoom_vulkan stdc++fs)
endif ()
