add_library(SamX71Monitor STATIC)
target_sources(SamX71Monitor
  PRIVATE
  Monitor.c
  PUBLIC
  Monitor.h)
target_include_directories(SamX71Monitor
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SamX71Monitor
  PRIVATE common_build_options
          SAMX71::Runtime::Hal
          SAMX71::Runtime::Mocks)

add_format_target(SamX71Monitor)

set_target_properties(SamX71Monitor PROPERTIES OUTPUT_NAME "samx71monitor")
add_library(SAMX71::Runtime::Monitor ALIAS SamX71Monitor)
