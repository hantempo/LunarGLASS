include_directories (..)

add_library (Preprocessor STATIC
    PpAtom.cpp
    PpScanner.cpp
    PpTokens.cpp
    Pp.cpp
    PpContext.cpp
    PpMemory.cpp
    PpSymbols.cpp
)
