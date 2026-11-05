cmake_minimum_required(VERSION 3.20)
project(XtremeFork-Universal VERSION 1.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_SCRIPTHOOKV_REGISTER "Register a ScriptHookV script thread via ScriptHookV.lib" OFF)

add_library(XtremeTool SHARED
    src/dllmain.cpp
    src/Config.h
    src/Config.cpp
    src/Logger.h
    src/Logger.cpp
    src/Pattern.h
    src/Pattern.cpp
    src/Game.h
    src/Game.cpp
    inc/main.h
    inc/types.h
)

target_include_directories(XtremeTool PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# Output as XtremeTool.asi
set_target_properties(XtremeTool PROPERTIES
    OUTPUT_NAME "XtremeTool"
    SUFFIX ".asi"
    PREFIX ""
)

if(WIN32)
    target_link_libraries(XtremeTool PRIVATE psapi)
    if(USE_SCRIPTHOOKV_REGISTER)
        target_compile_definitions(XtremeTool PRIVATE USE_SCRIPTHOOKV_REGISTER)
        target_link_libraries(XtremeTool PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib/ScriptHookV.lib")
    endif()
endif()
