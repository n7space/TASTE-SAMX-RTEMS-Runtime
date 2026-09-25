add_library(SamX71Hal STATIC)
target_sources(SamX71Hal
  PRIVATE
  Hal.c
  PUBLIC
  Hal.h)
target_include_directories(SamX71Hal
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SamX71Hal
  PRIVATE     common_build_options
  PUBLIC      SAMV71::Nvic
              SAMV71::Pio
              SAMV71::Pmc
              SAMV71::Tic
              SAMV71::Wdt
              SAMV71::Utils
              SAMX71::Runtime::Mocks
              SAMX71::Runtime::Core
              SAMX71::Runtime::BootHelper
              SAMV71::Uart)

add_format_target(SamX71Hal)

set_target_properties(SamX71Hal PROPERTIES OUTPUT_NAME "samx71hal")
add_library(SAMX71::Runtime::Hal ALIAS SamX71Hal)
