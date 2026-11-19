set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Add the ARM GNU toolchain bin directory to PATH before configuring.
find_program(CMAKE_C_COMPILER NAMES arm-none-eabi-gcc REQUIRED)
set(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}")
find_program(CMAKE_OBJCOPY NAMES arm-none-eabi-objcopy REQUIRED)
find_program(CMAKE_SIZE NAMES arm-none-eabi-size REQUIRED)

set(CMAKE_EXECUTABLE_SUFFIX_C ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_ASM ".elf")

# _INIT avoids appending flags repeatedly when CMake reloads the toolchain.
set(CMAKE_C_FLAGS_INIT "-mcpu=cortex-m3 -mthumb -Wall -fdata-sections -ffunction-sections -fstack-usage")
set(CMAKE_ASM_FLAGS_INIT "-mcpu=cortex-m3 -mthumb -x assembler-with-cpp -MMD -MP")
set(CMAKE_C_FLAGS_DEBUG_INIT "-O0 -g3")
set(CMAKE_C_FLAGS_RELEASE_INIT "-Os -g0")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-mcpu=cortex-m3 -mthumb")

set(TOOLCHAIN_LINK_LIBRARIES m)
