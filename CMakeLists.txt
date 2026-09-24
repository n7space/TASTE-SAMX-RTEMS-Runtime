cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

option(TARGET_SAMV71Q21 "Compile code for SamV71Q21" OFF)
option(TARGET_SAMV70F20 "Compile code form SamRH71F20" OFF)

if(TARGET_SAMV71Q21)
    if(TARGET_SAMV70F20)
        message(FATAL_ERROR "Options TARGET_SAMV71Q21 and TARGET_SAMV70F20 cannot be both enabled")
    endif()
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/toolchains/samv71.cmake)
elseif(TARGET_SAMV70F20)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/toolchains/samrh71.cmake)
else()
    message(FATAL_ERROR "One of the TARGET_SAMV71Q21 and TARGET_SAMV70F20 options shall be enabled")
endif()

project(Samv71Rtems VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

option(TOOLS_ENABLE_CPPCHECK "Enable Cppcheck" OFF)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(MessageColors)
include(LoggingHelper)
include(ClangFormat)
include(Doxygen)

add_library(common_build_options INTERFACE)

include(CommonBuildOptions)
include(Sanitizers)

option(TASTE_SAMV71_DRIVERS_WARNINGS_AS_ERRORS
       "Treat compiler warnings as errors"
       TRUE)

if(TASTE_SAMV71_DRIVERS_WARNINGS_AS_ERRORS)
    log_option_enabled("warnings as errors")
    set(CLANG_WARNINGS -Werror)
endif()

set(CLANG_WARNINGS ${CLANG_WARNINGS}
                   -Wall
                   -Wextra
                   -Wpedantic
                   -Wshadow
                   -Wcast-align
                   -Wunused
                   -Wsign-conversion
                   -Wnull-dereference
                   -Wdouble-promotion
                   -Wformat=2)

set(GCC_WARNINGS ${CLANG_WARNINGS}
                 -Wimplicit-function-declaration
                 -Wmisleading-indentation)

if(TOOLS_ENABLE_CPPCHECK)
    find_program(CPPCHECK_EXE cppcheck REQUIRED)
    if(CPPCHECK_EXE MATCHES "-NOTFOUND")
        log_program_missing("cppcheck")
    else()
        log_option_enabled("cppcheck")
    endif()
else()
    log_option_disabled("cppcheck")
endif()

log_info(${CMAKE_C_COMPILER_ID})

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(PROJECT_WARNINGS ${CLANG_WARNINGS})
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(PROJECT_WARNINGS ${GCC_WARNINGS})
endif()

target_compile_options(common_build_options INTERFACE ${PROJECT_WARNINGS})

if(TOOLS_ENABLE_CPPCHECK)
    set(CMAKE_C_CPPCHECK ${CPPCHECK_EXE}; --error-exitcode=1; --check-level=exhaustive; --language=c; --std=c99; --inline-suppr; --enable=all; --suppress=unmatchedSuppression; --suppress=missingIncludeSystem; --suppress=unusedFunction; --suppress=staticFunction; --suppress=checkersReport;)
endif()

add_library(bsp_build_options INTERFACE)
target_compile_definitions(bsp_build_options
    INTERFACE   USE_UART_IO
    LOW_LEVEL_IO_UART_ID=4)

include_directories(SYSTEM
    /opt/taste-rtems-qdp-arm/arm-rtems6/n7sbsp-samv71q21/lib/include
    /opt/taste-rtems-qdp-arm/include)

include(FetchContent)
FetchContent_Declare(arm-bsp SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/arm-bsp" SYSTEM EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(arm-bsp)
set_target_properties(Samv71Nvic PROPERTIES C_CPPCHECK "" CXX_CPPCHECK "" SKIP_LINTING ON)
set_target_properties(Samv71Wdt PROPERTIES C_CPPCHECK "" CXX_CPPCHECK "" SKIP_LINTING ON)
set_target_properties(Samv71Fpu PROPERTIES C_CPPCHECK "" CXX_CPPCHECK "" SKIP_LINTING ON)
set_target_properties(Samv71Sdramc PROPERTIES C_CPPCHECK "" CXX_CPPCHECK "" SKIP_LINTING ON)
set_target_properties(Samv71Scb PROPERTIES C_CPPCHECK "" CXX_CPPCHECK "" SKIP_LINTING ON)
set_target_properties(Samv71Rstc PROPERTIES C_CPPCHECK "" CXX_CPPCHECK "" SKIP_LINTING ON)
set_target_properties(Samv71Pio PROPERTIES C_CPPCHECK "" CXX_CPPCHECK "" SKIP_LINTING ON)
set_target_properties(Samv71Pmc PROPERTIES C_CPPCHECK "" CXX_CPPCHECK "" SKIP_LINTING ON)
set_target_properties(Samv71Tic PROPERTIES C_CPPCHECK "" CXX_CPPCHECK "" SKIP_LINTING ON)
set_target_properties(Samv71Uart PROPERTIES C_CPPCHECK "" CXX_CPPCHECK "" SKIP_LINTING ON)
set_target_properties(Samv71Utils PROPERTIES C_CPPCHECK "" CXX_CPPCHECK "" SKIP_LINTING ON)

add_subdirectory(src)
