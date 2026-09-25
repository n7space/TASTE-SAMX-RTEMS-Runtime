add_library(SamX71BootHelper STATIC)
target_sources(SamX71BootHelper
  PRIVATE
  BootHelper.c
  PUBLIC
  BootHelper.h)
target_include_directories(SamX71BootHelper
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SamX71BootHelper
  PRIVATE     common_build_options
  PUBLIC      SAMV71::Rstc)

add_format_target(SamX71BootHelper)

set_target_properties(SamX71BootHelper PROPERTIES OUTPUT_NAME "samx71BootHelper")
add_library(SAMX71::Runtime::BootHelper ALIAS SamX71BootHelper)
