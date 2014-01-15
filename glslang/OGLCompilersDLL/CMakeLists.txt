set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -D_ALT_NS=1")

include_directories (
    .
    ../glslang
    ../glslang/Include
    ../glslang/OSDependent/Linux
    ../glslang/MachineIndependent
)

add_library (InitializeDll STATIC
    InitializeDll.cpp
)
