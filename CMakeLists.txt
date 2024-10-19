cmake_minimum_required(VERSION 3.12)

project(
    SearchEngine
    VERSION 1.0
    DESCRIPTION "Simple Search Engine"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(BUILD_TOOL_OPTIONS "-Dgtest_disable_pthreads=OFF")

#find_package( OpenGL REQUIRED COMPONENTS )
#include_directories( ${OPENGL_INCLUDE_DIRS} )

add_subdirectory(indexer)
add_subdirectory(bin)
add_subdirectory(searcher)
add_subdirectory(imgui)
add_subdirectory(glfw)
enable_testing()
add_subdirectory(tests)

find_package(OpenGL REQUIRED)

cmake_policy(SET CMP0079 NEW)

add_compile_options(-Wall -Wextra -Wpedantic -Wvla -O2)

# # Add FlatBuffers directly to our build. This defines the `flatbuffers` target.
add_subdirectory(flatbuffers
                 ${CMAKE_CURRENT_BINARY_DIR}/flatbuffers-build
                  EXCLUDE_FROM_ALL)
 
# Now simply link against flatbuffers as needed to your already declared target.
# # The flatbuffers target carry header search path automatically if CMake > 2.8.11.
target_link_libraries(${PROJECT_NAME} PRIVATE flatbuffers)
target_link_libraries(${PROJECT_NAME} PRIVATE  glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)