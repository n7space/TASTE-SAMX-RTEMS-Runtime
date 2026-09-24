project(SamX71RuntimeXdmac VERSION 1.0.0 LANGUAGES C)

add_library(SamX71RuntimeXdmac STATIC)
target_sources(SamX71RuntimeXdmac
    PRIVATE     xdma_hardware_interface.c
                xdmac.c
                xdmad.c
    PUBLIC      xdmad.h
                xdmac.h
                xdma_hardware_interface.h)
target_include_directories(SamX71RuntimeXdmac
    PRIVATE     ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(SamX71RuntimeXdmac
    PRIVATE     common_build_options
                SAMV71::Pmc
                Samv71Scb
                SAMX71::Runtime::Hal)

add_format_target(SamX71RuntimeXdmac)

set_target_properties(SamX71RuntimeXdmac PROPERTIES OUTPUT_NAME "xdmac")
add_library(SAMX71::Runtime::Xdmac ALIAS SamX71RuntimeXdmac)
