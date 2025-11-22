cmake_minimum_required(VERSION 3.10.0)
project(Calculator VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(calculator 
    main.cpp
    calculator.cpp
    plugin_system.cpp
)

function(add_calculator_plugin plugin_name)
    add_library(${plugin_name} SHARED ${ARGN})
    
    target_include_directories(${plugin_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_features(${plugin_name} PRIVATE cxx_std_17)
    
    if(WIN32)
        set_target_properties(${plugin_name} PROPERTIES
            SUFFIX ".dll"
            PREFIX ""
        )
    endif()
    
    add_custom_command(TARGET ${plugin_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:calculator>/plugins
        COMMAND ${CMAKE_COMMAND} -E copy 
            $<TARGET_FILE:${plugin_name}> 
            $<TARGET_FILE_DIR:calculator>/plugins/
        COMMENT "Copying ${plugin_name} to calculator plugins directory"
    )
endfunction()

add_calculator_plugin(func1 plugins/func1.cpp)
add_calculator_plugin(func2 plugins/func2.cpp)

message("=== BUILD INFO ===")
message("All files will be in: ${CMAKE_BINARY_DIR}/bin")
message("Calculator: calculator.exe")