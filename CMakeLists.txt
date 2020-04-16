cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

# ---- Project ----

# Note: update this to your new project's name and version
project(Sortnet
  VERSION 1.0
  LANGUAGES CXX
)

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif()

# --- Import tools ----

include(cmake/tools.cmake)

# ---- Add dependencies via CPM ----
# see https://github.com/TheLartians/CPM.cmake for more info

include(cmake/CPM.cmake)

# PackageProject.cmake will be used to make our target installable
CPMAddPackage(
  NAME PackageProject.cmake
  GITHUB_REPOSITORY TheLartians/PackageProject.cmake
  VERSION 1.0
)

CPMAddPackage(
        NAME nlohmann_json
        VERSION 3.7.3
        # the git repo is incredibly large, so we download the archived include directory
        URL https://github.com/nlohmann/json/releases/download/v3.7.3/include.zip
        URL_HASH SHA256=87b5884741427220d3a33df1363ae0e8b898099fbc59f1c451113f6732891014
)

if (nlohmann_json_ADDED)
    add_library(nlohmann_json INTERFACE IMPORTED)
    target_include_directories(nlohmann_json INTERFACE ${nlohmann_json_SOURCE_DIR}/include)
endif()

CPMAddPackage(
        NAME boost
        VERSION 1.70.0
        URL https://dl.bintray.com/boostorg/release/1.70.0/source/boost_1_70_0.tar.gz
        URL_HASH SHA256=882b48708d211a5f48e60b0124cf5863c1534cd544ecd0664bb534a4b5d506e9
)

if (boost_ADDED)
    add_library(boost INTERFACE IMPORTED)
    target_include_directories(boost INTERFACE "${boost_SOURCE_DIR}")
endif()

# ---- Add source files ----

# Note: globbing sources is considered bad practice as CMake's generators may not detect new files automatically.
# Keep that in mind when changing files, or explicitly mention them here.
FILE(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/**/**/*.h")
FILE(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.cpp")

# ---- Create library ----

# Note: for header-only libraries change all PUBLIC flags to INTERFACE and create an interface target:
# add_library(Greeter INTERFACE)  
# set_target_properties(Greeter PROPERTIES INTERFACE_COMPILE_FEATURES cxx_std_17)

set(CMAKE_REQUIRED_FLAGS -fconcepts)

add_library(Sortnet ${headers} ${sources})

set_target_properties(Sortnet PROPERTIES CXX_STANDARD 20)

# beeing a cross-platform target, we enforce enforce standards conformance on MSVC
target_compile_options(Sortnet PUBLIC "$<$<BOOL:${MSVC}>:/permissive->")

# Link dependencies (if required)
target_link_libraries(Sortnet PUBLIC nlohmann_json boost)

target_include_directories(Sortnet
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
  PRIVATE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
)
