cmake_minimum_required(VERSION 3.0.2)
project (inMyRoom_vulkan)

add_subdirectory   (../Anvil "${CMAKE_CURRENT_BINARY_DIR}/anvil")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package (Threads REQUIRED)

target_include_directories(Anvil PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/anvil/include")


include_directories(../Anvil/include                                                   #Include Anvil
                    ../MathGeoLib/src                                                  #Include MathGeoLib
                    ../tinygltf                                                        #Include tinyglTF
                    ../Configuru                                                       #Include configuru
                    ../glm                                                             #Include glm
                    ${inMyRoom_vulkan_SOURCE_DIR}/include)
					
SET (SRC      "${inMyRoom_vulkan_SOURCE_DIR}/include/Engine.h"
              "${inMyRoom_vulkan_SOURCE_DIR}/include/glTFenum.h"
              "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics.h"
              "${inMyRoom_vulkan_SOURCE_DIR}/include/hash_combine.h"
              "${inMyRoom_vulkan_SOURCE_DIR}/include/InputManager.h"
              "${inMyRoom_vulkan_SOURCE_DIR}/include/MaterialsTextures.h"
              "${inMyRoom_vulkan_SOURCE_DIR}/include/MeshesPrimitives.h"
              "${inMyRoom_vulkan_SOURCE_DIR}/include/MovementBaseClass.h"
              "${inMyRoom_vulkan_SOURCE_DIR}/include/NaiveCamera.h"
              "${inMyRoom_vulkan_SOURCE_DIR}/include/NodesMeshes.h"
              "${inMyRoom_vulkan_SOURCE_DIR}/include/PrimitivesMaterials.h"
              "${inMyRoom_vulkan_SOURCE_DIR}/include/PrimitivesPipelines.h"
              "${inMyRoom_vulkan_SOURCE_DIR}/include/PrimitivesShaders.h"
              "${inMyRoom_vulkan_SOURCE_DIR}/include/SceneNodes.h"
              "${inMyRoom_vulkan_SOURCE_DIR}/include/WindowWithAsyncInput.h"
              "${inMyRoom_vulkan_SOURCE_DIR}/src/Engine.cpp"
              "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics.cpp"
              "${inMyRoom_vulkan_SOURCE_DIR}/src/InputManager.cpp"
              "${inMyRoom_vulkan_SOURCE_DIR}/src/MaterialsTextures.cpp"
              "${inMyRoom_vulkan_SOURCE_DIR}/src/MeshesPrimitives.cpp"
              "${inMyRoom_vulkan_SOURCE_DIR}/src/MovementBaseClass.cpp"
              "${inMyRoom_vulkan_SOURCE_DIR}/src/NaiveCamera.cpp"
              "${inMyRoom_vulkan_SOURCE_DIR}/src/NodesMeshes.cpp"
              "${inMyRoom_vulkan_SOURCE_DIR}/src/PrimitivesMaterials.cpp"
              "${inMyRoom_vulkan_SOURCE_DIR}/src/PrimitivesPipelines.cpp"
              "${inMyRoom_vulkan_SOURCE_DIR}/src/PrimitivesShaders.cpp"
              "${inMyRoom_vulkan_SOURCE_DIR}/src/SceneNodes.cpp"
              "${inMyRoom_vulkan_SOURCE_DIR}/src/WindowWithAsyncInput.cpp"
              "${inMyRoom_vulkan_SOURCE_DIR}/src/main.cpp")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${inMyRoom_vulkan_SOURCE_DIR})

add_executable (inMyRoom_vulkan ${SRC})

add_dependencies (inMyRoom_vulkan Anvil)

if (WIN32)
    target_link_libraries(inMyRoom_vulkan Anvil)
else()
    target_link_libraries(inMyRoom_vulkan Anvil dl)
endif()

if (UNIX)
    target_link_libraries(inMyRoom_vulkan stdc++fs)
endif()

target_link_libraries (inMyRoom_vulkan Threads::Threads)
