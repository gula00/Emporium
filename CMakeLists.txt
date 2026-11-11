cmake_minimum_required(VERSION 3.16)
project(EmporiumReconstruction LANGUAGES CXX)

set(CHAI3D_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../chai3d-src" CACHE PATH "CHAI3D source directory")
set(CHAI3D_BUILD "${CMAKE_CURRENT_SOURCE_DIR}/../build-chai3d" CACHE PATH "Built CHAI3D directory")
set(ODE_BUILD "${CMAKE_CURRENT_SOURCE_DIR}/../build-ode" CACHE PATH "Built CHAI3D ODE directory")
set(GEL_BUILD "${CMAKE_CURRENT_SOURCE_DIR}/../build-gel" CACHE PATH "Built CHAI3D GEL directory")

foreach(required_file
    "${CHAI3D_BUILD}/Release/chai3d.lib"
    "${CHAI3D_BUILD}/extras/GLFW/Release/glfw.lib"
    "${ODE_BUILD}/Release/chai3d-ODE.lib"
    "${GEL_BUILD}/Release/chai3d-GEL.lib")
    if(NOT EXISTS "${required_file}")
        message(FATAL_ERROR "Missing dependency: ${required_file}")
    endif()
endforeach()

set(APP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin/win-x64")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${APP_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${APP_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${APP_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${APP_DIR}")

function(add_scene name)
    add_executable(${name} "src/${name}.cpp")
    target_compile_features(${name} PRIVATE cxx_std_11)
    target_compile_definitions(${name} PRIVATE GLEW_STATIC _CRT_SECURE_NO_DEPRECATE)
    target_include_directories(${name} PRIVATE
        "${CHAI3D_ROOT}/src"
        "${CHAI3D_ROOT}/externals/Eigen"
        "${CHAI3D_ROOT}/externals/glew/include"
        "${CHAI3D_ROOT}/extras/GLFW/include")
    if(name STREQUAL "scene4")
        target_include_directories(${name} PRIVATE "${CHAI3D_ROOT}/modules/GEL/src")
        target_link_libraries(${name} PRIVATE "${GEL_BUILD}/Release/chai3d-GEL.lib")
    elseif(name STREQUAL "scene5")
        target_compile_definitions(${name} PRIVATE ODE_LIB dDOUBLE)
        target_include_directories(${name} PRIVATE
            "${CHAI3D_ROOT}/modules/ODE/src"
            "${CHAI3D_ROOT}/modules/ODE/externals/ODE/include")
        target_link_libraries(${name} PRIVATE "${ODE_BUILD}/Release/chai3d-ODE.lib")
    endif()
    target_link_libraries(${name} PRIVATE
        "${CHAI3D_BUILD}/Release/chai3d.lib"
        "${CHAI3D_BUILD}/extras/GLFW/Release/glfw.lib"
        opengl32 glu32 winmm)
endfunction()

foreach(n RANGE 1 6)
    add_scene("scene${n}")
endforeach()

add_executable(EmporiumReconstruction src/launcher.cpp)
target_compile_features(EmporiumReconstruction PRIVATE cxx_std_11)
