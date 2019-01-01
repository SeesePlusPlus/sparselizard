cmake_minimum_required(VERSION 3.1)

set (CMAKE_CXX_STANDARD 11)

if (UNIX AND NOT APPLE)
  set (LINUX TRUE)
endif ()

project (sparselizard)

file (GLOB_RECURSE CPP_FILES "src/*.cpp")
file (GLOB_RECURSE H_FILES "src/*.h")

#add_library (${PROJECT_NAME} SHARED ${CPP_FILES})
add_library (${PROJECT_NAME}_static STATIC ${CPP_FILES})
set_target_properties (${PROJECT_NAME}_static PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

if (LINUX)
  set (INCLUDE_NAME "linux2")
elseif (APPLE)
  set (INCLUDE_NAME "darwin")
endif ()

set (DEPENDENCY_PREFIX ~/SLlibs)
set (PETSC_INCLUDE ${DEPENDENCY_PREFIX}/petsc/include)
set (PETSC_ARCH ${DEPENDENCY_PREFIX}/petsc/arch-${INCLUDE_NAME}-c-opt)
set (SLEPC_INCLUDE ${DEPENDENCY_PREFIX}/slepc/include)
set (SLEPC_ARCH ${DEPENDENCY_PREFIX}/slepc/arch-${INCLUDE_NAME}-c-opt)

set (DEPENDENCY_INCLUDES
  /usr/local/include
)

set (PROJECT_INCLUDES
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/src/field
  ${PROJECT_SOURCE_DIR}/src/expression
  ${PROJECT_SOURCE_DIR}/src/expression/operation
  ${PROJECT_SOURCE_DIR}/src/shapefunction
  ${PROJECT_SOURCE_DIR}/src/formulation
  ${PROJECT_SOURCE_DIR}/src/shapefunction/hierarchical
  ${PROJECT_SOURCE_DIR}/src/shapefunction/hierarchical/h1
  ${PROJECT_SOURCE_DIR}/src/shapefunction/hierarchical/hcurl
  ${PROJECT_SOURCE_DIR}/src/shapefunction/hierarchical/meca
  ${PROJECT_SOURCE_DIR}/src/gausspoint
  ${PROJECT_SOURCE_DIR}/src/shapefunction/lagrange
  ${PROJECT_SOURCE_DIR}/src/mesh
  ${PROJECT_SOURCE_DIR}/src/mesh/gmsh
  ${PROJECT_SOURCE_DIR}/src/resolution
  ${PROJECT_SOURCE_DIR}/src/geometry
)

include_directories (
  ${PROJECT_INCLUDES}
  ${DEPENDENCY_INCLUDES}
)

#install (TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include)
install (TARGETS ${PROJECT_NAME}_static ARCHIVE DESTINATION lib)
install (FILES ${H_FILES} DESTINATION include/${PROJECT_NAME})
#install (DIRECTORY src/ DESTINATION include/${PROJECT_NAME} FILES_MATCHING PATTERN "*.h")

#add_executable (${PROJECT_NAME} src/main.cpp)