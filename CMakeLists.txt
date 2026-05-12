cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/toolchains/samv71.cmake)

project(Samv71Rtems VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(MessageColors)
include(LoggingHelper)
include(ClangFormat)

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
                 -Wmisleading-indentation
                 -Wduplicated-cond
                 -Wduplicated-branches
                 -Wlogical-op)

log_info(${CMAKE_C_COMPILER_ID})

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(PROJECT_WARNINGS ${CLANG_WARNINGS})
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(PROJECT_WARNINGS ${GCC_WARNINGS})
endif()

target_compile_options(common_build_options INTERFACE ${PROJECT_WARNINGS})

add_library(bsp_build_options INTERFACE)
target_compile_definitions(bsp_build_options
    INTERFACE   USE_UART_IO
  LOW_LEVEL_IO_UART_ID=4)

include_directories(SYSTEM
    /opt/taste-rtems-qdp-arm/arm-rtems6/n7sbsp-samv71q21/lib/include
    /opt/taste-rtems-qdp-arm/include)

add_subdirectory(arm-bsp/src/Nvic)
add_subdirectory(arm-bsp/src/Wdt)
add_subdirectory(arm-bsp/src/Fpu)
add_subdirectory(arm-bsp/src/Sdramc)
add_subdirectory(arm-bsp/src/Scb)
add_subdirectory(arm-bsp/src/Rstc)
add_subdirectory(arm-bsp/src/Pio)
add_subdirectory(arm-bsp/src/Pmc)
add_subdirectory(arm-bsp/src/Tic)
add_subdirectory(arm-bsp/src/Uart)
add_subdirectory(arm-bsp/src/Utils)
add_subdirectory(src)
