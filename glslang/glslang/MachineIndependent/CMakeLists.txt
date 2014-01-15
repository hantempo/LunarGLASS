set (LOCAL_DIR ${PROJECT_SOURCE_DIR}/glslang/MachineIndependent)

add_custom_command (
    OUTPUT ${LOCAL_DIR}/gen_glslang_tab.cpp ${LOCAL_DIR}/glslang_tab.cpp.h
    COMMAND dos2unix glslang.y
    COMMAND bison -t -v -d glslang.y
	COMMAND mv glslang.tab.c gen_glslang_tab.cpp
	COMMAND mv glslang.tab.h glslang_tab.cpp.h
    DEPENDS glslang.y
    WORKING_DIRECTORY ${LOCAL_DIR}
)

include_directories (
    .
    ..
    ../..
    ../Include
    ../Public
    ../OSDependent/Linux
    ../../OGLCompilersDLL
)

add_library (MachineIndependent STATIC
    gen_glslang_tab.cpp
    Initialize.cpp
    IntermTraverse.cpp
	Intermediate.cpp
    ParseHelper.cpp
    PoolAlloc.cpp
	RemoveTree.cpp
    ShaderLang.cpp
    SymbolTable.cpp
    intermOut.cpp
	parseConst.cpp
    InfoSink.cpp
    Versions.cpp
    Constant.cpp
    Scan.cpp
    reflection.cpp
    linkValidate.cpp
    limits.cpp
)
