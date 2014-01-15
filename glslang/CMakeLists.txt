cmake_minimum_required (VERSION 2.8)

project (glslang)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif ()

add_subdirectory (OGLCompilersDLL)
add_subdirectory (glslang/MachineIndependent)
add_subdirectory (glslang/MachineIndependent/preprocessor)
add_subdirectory (glslang/GenericCodeGen)
add_subdirectory (glslang/OSDependent/Linux)

add_library (glslang SHARED
)

target_link_libraries (glslang
    -rdynamic
    -Wl,--whole-archive
    InitializeDll
    MachineIndependent
    Preprocessor
    CodeGen
    Ossource
    -Wl,--no-whole-archive
)

include_directories (
    glslang/OSDependent/Linux
)

add_executable (glslangValidator
    StandAlone/StandAlone.cpp)

target_link_libraries (glslangValidator
    glslang
    pthread
    m
    stdc++
)
