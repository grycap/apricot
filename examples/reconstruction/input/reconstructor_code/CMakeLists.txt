
#----------------------------------------------------------------------------
# Comprovem la minima versio del CMake
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

# Nom de projecte
project(reconstructor LANGUAGES CXX)

#-----------------------------------------------------------------------------
#Comprovem  si hem de compilar amb pedantic -Wall etc
option(WITH_WARNINGS "Build Reconstructor with all warnings " ON)
#Comprovem si hem de compilar amb OMP
option(WITH_OMP "Build Reconstructor OMP " OFF)
#-----------------------------------------------------------------------------

# Opcions compilacio
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -std=c++11")
#-----------------------------------------------------------------------------

# Afegim el directori on es troben els include del nostre programa
include_directories(${PROJECT_SOURCE_DIR}/src/includes)

#-----------------------------------------------------------------------------

# Busquem on es troben els .h i els .cpp
file(GLOB RECONSTRUCTOR_SRC ${PROJECT_SOURCE_DIR}/src/code/*.cpp)
file(GLOB RECONSTRUCTOR_H ${PROJECT_SOURCE_DIR}/src/includes/*.h)

#-----------------------------------------------------------------------------

# Setup dependencies

if(WITH_OMP)
  #OpenMP package
  add_definitions(-D_RECONSTRUCT_OMP_=1) #enable OMP in code (equivalent to #define _RECONSTRUCT_OMP_ 1)
  find_package(OpenMP REQUIRED)
  include_directories(${OPENMP_INCLUDE_DIRS})
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
  
endif()
  
# Compile projectors

add_library(projectors STATIC ${PROJECT_SOURCE_DIR}/src/code/projectors.cpp)

# Compile input

add_library(input STATIC ${PROJECT_SOURCE_DIR}/src/code/input.cpp)
target_link_libraries(input projectors)

# Compile reconstruction

add_library(reconstruction STATIC ${PROJECT_SOURCE_DIR}/src/code/reconstruction.cpp)
target_link_libraries(reconstruction input)

#----------------------------------------------------------------------------
# Afegim l'executable i linquem les llibreries
#

add_executable(reconstructor ${PROJECT_SOURCE_DIR}/src/code/main.cpp)

# Target libs
target_link_libraries(reconstructor projectors)
target_link_libraries(reconstructor reconstruction)
target_link_libraries(reconstructor input)


if(WITH_OMP)
  #OpenMP libs
  target_link_libraries(reconstructor ${OPENMP_LIBRARY})
endif()


if(WITH_WARNINGS)
  target_compile_options(projectors PRIVATE -march=native -O2 -W -Wall -Wextra -Wpedantic -Wshadow)
  target_compile_options(reconstruction PRIVATE -march=native -O2 -W -Wall -Wextra -Wpedantic -Wshadow)  
  target_compile_options(reconstructor PRIVATE -march=native -O2 -W -Wall -Wextra -Wpedantic -Wshadow)

else()
  target_compile_options(projectors PRIVATE -march=native -O2)
  target_compile_options(reconstruction PRIVATE -march=native -O2)
  target_compile_options(reconstructor PRIVATE -march=native -O2)
endif()
