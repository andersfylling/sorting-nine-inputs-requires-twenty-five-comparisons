cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

project(SortnetApp
  LANGUAGES CXX
)

# ---- Dependencies ----

include(../cmake/CPM.cmake)


CPMAddPackage(
  NAME cxxopts
  GITHUB_REPOSITORY jarro2783/cxxopts
  VERSION 2.2.0
  OPTIONS
    "CXXOPTS_BUILD_EXAMPLES Off"
    "CXXOPTS_BUILD_TESTS Off"
)

CPMAddPackage(
  NAME tabulate
  GITHUB_REPOSITORY p-ranav/tabulate
  GIT_TAG origin/master
)

CPMAddPackage(
  NAME progresscpp
  GITHUB_REPOSITORY andersfylling/progress-cpp
  GIT_TAG origin/master
)

CPMAddPackage(
  NAME libpopcnt
  GITHUB_REPOSITORY kimwalisch/libpopcnt
  VERSION 2.3
)

CPMAddPackage(
  NAME Sortnet
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..
)

# ---- Configure compile time c++ parameters ----

if (NOT SORTNET_PARAM_N)
    set(SORTNET_PARAM_N 0)
endif()
if (NOT SORTNET_PARAM_K)
    set(SORTNET_PARAM_K 0)
endif()
if (NOT SORTNET_PARAM_THREADS)
    set(SORTNET_PARAM_THREADS 0)
endif()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/settings.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/settings.h")

# ---- Create standalone executable ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
FILE(GLOB headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

add_executable(SortnetApp ${headers} ${sources})

set_target_properties(SortnetApp PROPERTIES
  CXX_STANDARD 20
  COMPILE_FLAGS "-Wall -Wextra -Wno-sign-compare -Wno-narrowing"
  OUTPUT_NAME "SortnetApp"
) #  -fsanitize=undefined

# enable compiler warnings
if (NOT TEST_INSTALLED_VERSION)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_compile_options(Sortnet PUBLIC -Wall -Wextra -Werror)
    elseif(MSVC)
        target_compile_options(Sortnet PUBLIC /W4 /WX)
    endif()
endif()


target_link_libraries(SortnetApp Sortnet tabulate progresscpp cxxopts)
