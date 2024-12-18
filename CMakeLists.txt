CMAKE_MINIMUM_REQUIRED(VERSION 3.12)

set(PROJECT Terminal)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(${PROJECT} C CXX)

add_subdirectory(${CMAKE_SOURCE_DIR}/dependencies/glfw)

file(GLOB_RECURSE SOURCES
  ${CMAKE_SOURCE_DIR}/src/*.cpp
  ${CMAKE_SOURCE_DIR}/dependencies/glad/*.c
  ${CMAKE_SOURCE_DIR}/dependencies/imgui/*.cpp
)

include_directories(
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/dependencies
  ${CMAKE_SOURCE_DIR}/dependencies/imgui
)

add_executable(${PROJECT} ${SOURCES})

target_link_libraries(${PROJECT} glfw GL)

add_custom_target(copy_compile_commands ALL
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/build/compile_commands.json ${CMAKE_SOURCE_DIR}
COMMENT "Copied compile_commands.json" 
)


add_dependencies(${PROJECT} copy_compile_commands)
