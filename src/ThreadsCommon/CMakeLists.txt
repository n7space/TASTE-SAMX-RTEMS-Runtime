add_library(SamX71ThreadsCommon STATIC)
target_sources(SamX71ThreadsCommon
  PRIVATE
  ThreadsCommon.c
  PUBLIC
  ThreadsCommon.h)
target_include_directories(SamX71ThreadsCommon
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SamX71ThreadsCommon
  PRIVATE common_build_options
          SAMX71::Runtime::Hal
          SAMX71::Runtime::Monitor
          SAMX71::Runtime::Mocks)

add_format_target(SamX71ThreadsCommon)

set_target_properties(SamX71ThreadsCommon PROPERTIES OUTPUT_NAME "samx71threadscommon")
add_library(SAMX71::Runtime::ThreadsCommon ALIAS SamX71ThreadsCommon)
