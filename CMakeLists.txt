cmake_minimum_required(VERSION 2.8)

if (UNIX AND NOT APPLE)
  set (LINUX TRUE)
endif ()

project (sparselizard)

file (GLOB CPP_FILES "src/*.cpp" "src/**/*.cpp")
file (GLOB H_FILES "src/*.h" "src/**/*.h")

add_library (${PROJECT_NAME} SHARED ${CPP_FILES})
add_library (${PROJECT_NAME}Static STATIC ${CPP_FILES})
set_target_properties (${PROJECT_NAME}Static PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

if (LINUX)
  set (INCLUDE_NAME "linux2")
elseif (APPLE)
  set (INCLUDE_NAME "darwin")
endif ()

include_directories (
  PUBLIC
    "${PROJECT_SOURCE_DIR}/src"
    "${PROJECT_SOURCE_DIR}/src/field"
    "${PROJECT_SOURCE_DIR}/src/expression"
    "${PROJECT_SOURCE_DIR}/src/expression/operation"
    "${PROJECT_SOURCE_DIR}/src/shapefunction"
    "${PROJECT_SOURCE_DIR}/src/formulation"
    "${PROJECT_SOURCE_DIR}/src/shapefunction/hierarchical"
    "${PROJECT_SOURCE_DIR}/src/shapefunction/hierarchical/h1"
    "${PROJECT_SOURCE_DIR}/src/shapefunction/hierarchical/hcurl"
    "${PROJECT_SOURCE_DIR}/src/shapefunction/hierarchical/meca"
    "${PROJECT_SOURCE_DIR}/src/gausspoint"
    "${PROJECT_SOURCE_DIR}/src/shapefunction/lagrange"
    "${PROJECT_SOURCE_DIR}/src/mesh"
    "${PROJECT_SOURCE_DIR}/src/mesh/gmsh"
    "${PROJECT_SOURCE_DIR}/src/resolution"
    "${PROJECT_SOURCE_DIR}/src/geometry"
  PRIVATE
    "~/SLlibs/petsc/include/petsc/mpiuni"
    "~/SLlibs/petsc/include/"
    "~/SLlibs/slepc/include"
    "~/SLlibs/petsc/arch-${INCLUDE_NAME}-c-opt/externalpackages/git.openblas"
    "~/SLlibs/petsc/arch-${INCLUDE_NAME}-c-opt/include/"
    "~/SLlibs/slepc/arch-${INCLUDE_NAME}-c-opt/include"
)

install (TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)
install (DIRECTORY src/ DESTINATION lib/${PROJECT_NAME} FILES_MATCHING PATTERN "*.h")

#add_executable (${PROJECT_NAME} src/main.cpp)