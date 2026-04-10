add_library(SamRH71Core STATIC)
target_sources(SamRH71Core
  PRIVATE
  SamRH71Core.c
  PUBLIC
  SamRH71Core.h)
target_include_directories(SamRH71Core
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SamRH71Core
  PRIVATE SAMRH71::Pmc
          SAMRH71::Wdt
          SAMRH71::Tic)

add_format_target(SamRH71Core)

set_target_properties(SamRH71Core PROPERTIES OUTPUT_NAME "samrh71core")
add_library(SAMRH71::Runtime::Core ALIAS SamRH71Core)
