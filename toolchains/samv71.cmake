set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER arm-rtems6-gcc)
set(CMAKE_CXX_COMPILER arm-rtems6-g++)

set(CMAKE_C_FLAGS
"-mcpu=cortex-m7 \
-mfloat-abi=hard \
-mfpu=fpv5-d16 \
-mlittle-endian \
-mthumb \
-ffunction-sections \
-DN7S_TARGET_SAMV71Q21 \
-isystem /opt/taste-rtems-qdp-arm/arm-rtems6/n7sbsp-samv71q21/lib/include \
-isystem /opt/taste-rtems-qdp-arm/include")
set(CMAKE_CXX_FLAGS
"-mcpu=cortex-m7 \
-mfloat-abi=hard \
-mfpu=fpv5-d16 \
-mlittle-endian \
-mthumb \
-ffunction-sections \
-DN7S_TARGET_SAMV71Q21 \
-isystem /opt/taste-rtems-qdp-arm/arm-rtems6/n7sbsp-samv71q21/lib/include \
-isystem /opt/taste-rtems-qdp-arm/include")
set(CMAKE_EXE_LINKER_FLAGS
"-qnolinkcmds \
-Wl,-T${CMAKE_CURRENT_SOURCE_DIR}/linker_script/linkcmds.intsram \
-Wl,-L${CMAKE_CURRENT_SOURCE_DIR}/linker_script \
-Wl,-L/opt/taste-rtems-qdp-arm/arm-rtems6/n7sbsp-samv71q21/lib \
-qrtems \
-Wl,--start-group \
-lrtemscpu \
-lrtemsbsp \
-lgcc \
-Wl,--end-group \
-Wl,--gc-sections")
