# CMakeList.txt : CMake project for OpenGL_CMake_Template, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("OpenGL_CMake_Template")

# Include paths
include_directories(${CMAKE_SOURCE_DIR}/libs/include)

# Linker library paths
link_directories(${CMAKE_SOURCE_DIR}/libs/lib)

# Add your executable
add_executable(MyOpenGLProject main.cpp)

# Link the libraries
target_link_libraries(MyOpenGLProject
    glew32
    freeglut
    opengl32
)

add_custom_command(TARGET MyOpenGLProject POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/libs/bin/glew32.dll"
        "${CMAKE_SOURCE_DIR}/libs/bin/freeglut.dll"
        $<TARGET_FILE_DIR:MyOpenGLProject>
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/vertex_shader.glsl" $<TARGET_FILE_DIR:MyOpenGLProject>/vertex_shader.glsl
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/fragment_shader.glsl" $<TARGET_FILE_DIR:MyOpenGLProject>/fragment_shader.glsl
)
