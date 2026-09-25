add_library(SamX71Core STATIC)
target_sources(SamX71Core
  PRIVATE
  SamV71Core.c
  PUBLIC
  SamV71Core.h)
target_include_directories(SamX71Core
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SamX71Core
  PRIVATE common_build_options
          SAMV71::Pmc
          SAMV71::Wdt
          SAMV71::Tic)

add_format_target(SamX71Core)

set_target_properties(SamX71Core PROPERTIES OUTPUT_NAME "samx71core")
add_library(SAMX71::Runtime::Core ALIAS SamX71Core)
