set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -D_ALT_NS=1")

include_directories (
    .
    ../..
    ../../Include
    ../../../OGLCompilersDLL
)

add_library (Ossource STATIC
    ossource.cpp)
