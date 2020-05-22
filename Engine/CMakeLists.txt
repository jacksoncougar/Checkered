# CMakeList.txt : CMake project for Engine, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.12)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")



message(STATUS "OKAY: ${PHYSX_COOKING_LIB}")
message(STATUS "OKAY: ${PHYSX_COMMON_LIB}")


set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(GLAD ../Include/glad/src/glad.c)

file(GLOB_RECURSE PHYSX_SNIPPETS ../Engine/Systems/Physics/snippet** *.h *.cpp)

set(SRC main.cpp main.h
	Scenarios/testworld.cpp Scenarios/testworld.hpp
        Systems/Engine.cpp Systems/Engine.h
        Components/tags.cpp Components/tags.h
        Components/Mesh.cpp Components/Mesh.h
        Components/SceneComponent.cpp Components/SceneComponent.h
        Systems/Debug/LiveReloadSystem.cpp Systems/Debug/LiveReloadSystem.h
        Components/ComponentBase.h Components/ComponentBase.cpp
        Systems/Physics/PhysicsSystem.cpp Systems/Physics/PhysicsSystem.h
        Systems/Rendering/RenderingSystem.cpp Systems/Rendering/RenderingSystem.h
        Systems/Rendering/RenderingBatch.h Systems/Rendering/RenderingBatch.cpp
        Systems/Pipeline/ShaderLoader.cpp Systems/Pipeline/ShaderLoader.h
        Systems/Camera/CameraSystem.cpp Systems/Camera/CameraSystem.h
        Systems/Input/InputSystem.cpp Systems/Input/InputSystem.h
        Systems/Pipeline/MeshLoader.h
        Systems/Physics/SimulationCallback.h Systems/Physics/SimulationCallback.cpp
        Components/Shader.cpp Components/Shader.h
        Components/ComponentId.cpp Components/ComponentId.h
        Components/ComponentId.tpp.h
        Components/Camera.cpp Components/Camera.h
        Systems/Rendering/Rendering.h
        Systems/Physics/FilterShader.cpp Systems/Physics/FilterShader.h
        Components/Plane.cpp Components/Plane.h
        Systems/Pipeline/EntityLoader.cpp Systems/Pipeline/EntityLoader.h
        Components/Model.cpp Components/Model.h
        Components/ReportCard.cpp Components/ReportCard.h
        Components/Passenger.cpp Components/Passenger.h
        Components/Vehicle.cpp Components/Vehicle.h
        Components/EventHandler.cpp Components/EventHandler.h
        Components/EventDelegate.cpp Components/EventDelegate.h
        Systems/Rendering/BillboardSystem.cpp Systems/Rendering/BillboardSystem.h
        Components/Billboard.cpp Components/Billboard.h
        Components/Component.h
        Components/Sound.h Components/Sound.cpp
        Systems/Events/Events.cpp Systems/Events/Events.h
        Systems/Pipeline/Library.cpp Systems/Pipeline/Library.h
        ${PHYSX_SNIPPETS}
        Systems/Pipeline/pipeline.cpp
        Systems/Pipeline/pipeline.hpp
        Systems/Damage/damagesystem.cpp
        Systems/Damage/damagesystem.hpp
        Components/damage.cpp Components/damage.hpp
        Systems/SystemInterface.hpp Systems/Component/scenecomponentsystem.cpp
        Systems/Component/scenecomponentsystem.hpp Components/physicspacket.cpp
        Components/physicspacket.hpp
        Systems/hornSystem/hornSystem.cpp
        Systems/hornSystem/hornSystem.h
        Systems/Vehicle/vehiclesystem.cpp Systems/Vehicle/vehiclesystem.hpp
        Systems/soundSystem/SoundSystem.cpp Systems/soundSystem/SoundSystem.h
        Components/WorldTransform.h Components/WorldTransform.cpp
        Components/texture.cpp Components/texture.hpp
        Components/material.cpp Components/material.hpp 
        Components/PhysicsActor.cpp Components/PhysicsActor.h
        Systems/Component/scenerysystem.cpp Systems/Component/scenerysystem.h
        Systems/EngineStore.cpp Systems/EngineStore.h
        Systems/Physics/VehicleFactory.cpp Systems/Physics/VehicleFactory.hpp
        Systems/Physics/VehicleFactoryFunctions.cpp Systems/Physics/VehicleFactoryFunctions.hpp
        Components/TransformData.cpp Components/TransformData.hpp
        Components/bitmap.cpp Components/bitmap.hpp
        Systems/Rendering/fontsystem.cpp Systems/Rendering/fontsystem.hpp
        Systems/Rendering/text.cpp Systems/Rendering/text.hpp
        Scenarios/start.cpp Scenarios/start.hpp
        Scenarios/scenariointerface.cpp Scenarios/scenariointerface.hpp
        Systems/EngineDebug.cpp Systems/EngineDebug.hpp Components/Node.cpp Components/Node.hpp Components/PlayerVehicle.cpp Components/PlayerVehicle.hpp Systems/Physics/Wheel.cpp Systems/Physics/Wheel.hpp)

# Add source to this project's executable.
add_executable(Engine
        ${GLAD}
        ${SRC})

set_property(TARGET Engine PROPERTY CXX_STANDARD 17)
set_property(TARGET Engine PROPERTY CMAKE_CXX_STANDARD_REQUIRED ON)

macro(remove_flag_from_target _target _flag)
    get_target_property(_target_cxx_flags ${_target} COMPILE_OPTIONS)
    if (_target_cxx_flags)
        list(REMOVE_ITEM _target_cxx_flags ${_flag})
        set_target_properties(${_target} PROPERTIES COMPILE_OPTIONS "${_target_cxx_flags}")
    endif ()
endmacro()


if (MSVC)
    remove_flag_from_target(Engine /W3)
    target_compile_options(Engine PRIVATE /W3)
else ()
    target_compile_options(Engine PRIVATE -Wall -Wextra -pedantic)
endif ()


# GAME ASSETS
# copy assets into build directory

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MSVC_COMPILE_FLAGS}")

FIND_LIBRARY(PHYSX_64_LIB  NAMES PhysX_64)
FIND_LIBRARY(PHYSXCHARACTERKINEMATIC_STATIC_64_LIB  NAMES PhysXCharacterKinematic_static_64)
FIND_LIBRARY(PHYSXCOMMON_64_LIB  NAMES PhysXCommon_64)
FIND_LIBRARY(PHYSXCOOKING_64_LIB  NAMES PhysXCooking_64)
FIND_LIBRARY(PHYSXEXTENSIONS_STATIC_64_LIB  NAMES PhysXExtensions_static_64)
FIND_LIBRARY(PHYSXFOUNDATION_64_LIB  NAMES PhysXFoundation_64)
FIND_LIBRARY(PHYSXPVDSDK_STATIC_64_LIB  NAMES PhysXPvdSDK_static_64)
FIND_LIBRARY(PHYSXTASK_STATIC_64_LIB  NAMES PhysXTask_static_64)
FIND_LIBRARY(PHYSXVEHICLE_STATIC_64_LIB  NAMES PhysXVehicle_static_64)


FIND_PATH(PHYSX_INCLUDE_DIRS PxPhysics.h PATH_SUFFIXES physx)
message(STATUS ${PHYSX_INCLUDE_DIRS})

message(STATUS ${PHYSX_64_LIB})
message(STATUS ${PHYSXCHARACTERKINEMATIC_STATIC_64_LIB})
message(STATUS ${PHYSXCOMMON_64_LIB})
message(STATUS ${PHYSXCOOKING_64_LIB})
message(STATUS ${PHYSXEXTENSIONS_STATIC_64_LIB})
message(STATUS ${PHYSXFOUNDATION_64_LIB})
message(STATUS ${PHYSXPVDSDK_STATIC_64_LIB})
message(STATUS ${PHYSXTASK_STATIC_64_LIB})
message(STATUS ${PHYSXVEHICLE_STATIC_64_LIB})


find_package(OpenAL CONFIG REQUIRED)
find_package(OpenGL REQUIRED COMPONENTS OpenGL)
find_package(glfw3 3 REQUIRED)

target_link_libraries(Engine
        PRIVATE glfw
        PRIVATE SOIL
        PRIVATE freetype

        PRIVATE OpenAL::OpenAL 
        PRIVATE OpenGL::GL 

        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party/assimp/x64-Debug/lib/assimp-vc142-mt.lib

        PRIVATE ${PHYSX_64_LIB}
        PRIVATE ${PHYSXCHARACTERKINEMATIC_STATIC_64_LIB}
        PRIVATE ${PHYSXCOMMON_64_LIB}
        PRIVATE ${PHYSXCOOKING_64_LIB}
        PRIVATE ${PHYSXEXTENSIONS_STATIC_64_LIB}
        PRIVATE ${PHYSXFOUNDATION_64_LIB}
        PRIVATE ${PHYSXPVDSDK_STATIC_64_LIB}
        PRIVATE ${PHYSXTASK_STATIC_64_LIB}
        PRIVATE ${PHYSXVEHICLE_STATIC_64_LIB}

       )

configure_file("../dlls/assimp-vc142-mt.dll" assimp-vc142-mt.dll COPYONLY)
configure_file("../dlls/glut32.dll" glut32.dll COPYONLY)

find_package(OpenGL REQUIRED)

target_include_directories(Engine SYSTEM PRIVATE ${OPENGL_INCLUDE_DIR}
        PRIVATE "Components"
        PRIVATE "Systems"
        PRIVATE "Scenarios"
        SYSTEM PRIVATE OpenAL::OpenAL
        PRIVATE ${PHYSX_INCLUDE_DIRS}
        SYSTEM PRIVATE "../Include/include"
        SYSTEM PRIVATE "../Include/glminclude"
        SYSTEM PRIVATE ../3rd-party/assimp/x64-Debug/include
        SYSTEM PRIVATE "../Include/glad/include")

add_custom_command(
        TARGET Engine POST_BUILD
        COMMENT "Copying assets to engine directory"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Assets
        $<TARGET_FILE_DIR:Engine>/Assets)

