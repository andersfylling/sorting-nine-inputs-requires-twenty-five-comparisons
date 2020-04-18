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

# ---- Create standalone executable ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
FILE(GLOB headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

add_executable(SortnetApp ${headers} ${sources})

set_target_properties(SortnetApp PROPERTIES
  CXX_STANDARD 20
  COMPILE_FLAGS "-Wall -pedantic -Wextra -D_GLIBCXX_ASSERTIONS -Wno-sign-compare -Wno-narrowing -fconcepts"
  OUTPUT_NAME "SortnetApp"
) #  -fsanitize=undefined

# enable compiler warnings
if (NOT TEST_INSTALLED_VERSION)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_compile_options(Sortnet PUBLIC -Wall -pedantic -Wextra -Werror -fconcepts)
    elseif(MSVC)
        target_compile_options(Sortnet PUBLIC /W4 /WX)
    endif()
endif()

set(Boost_USE_STATIC_LIBS ON)

target_link_libraries(SortnetApp Sortnet tabulate progresscpp cxxopts)
