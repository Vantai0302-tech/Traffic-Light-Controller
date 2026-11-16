cmake_minimum_required(VERSION 3.22)

# Select the embedded toolchain even when configuring without a preset.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/gcc-arm-none-eabi.cmake"
        CACHE FILEPATH "ARM GCC toolchain")
endif()

project(Traffic_Light_Controller LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

add_executable(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")

# STM32CubeMX sources: CMSIS, HAL, startup and Core runtime.
add_subdirectory(cmake/stm32cubemx)
target_link_libraries(${PROJECT_NAME} stm32cubemx)

# Add user sources here; keep the original Core/Inc and Core/Src structure.
target_sources(${PROJECT_NAME} PRIVATE
    Core/Src/scheduler.c
)

# Keep board-specific linker settings out of the compiler toolchain.
set(STM32_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/STM32F103xx_FLASH.ld")
target_link_options(${PROJECT_NAME} PRIVATE
    "-T${STM32_LINKER_SCRIPT}"
    --specs=nano.specs
    "-Wl,-Map=${PROJECT_NAME}.map"
    -Wl,--gc-sections
    -Wl,--print-memory-usage
)
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY LINK_DEPENDS "${STM32_LINKER_SCRIPT}")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND "${CMAKE_OBJCOPY}" -O ihex "$<TARGET_FILE:${PROJECT_NAME}>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.hex"
    COMMAND "${CMAKE_OBJCOPY}" -O binary "$<TARGET_FILE:${PROJECT_NAME}>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.bin"
    COMMAND "${CMAKE_SIZE}" "$<TARGET_FILE:${PROJECT_NAME}>"
    BYPRODUCTS "${PROJECT_NAME}.hex" "${PROJECT_NAME}.bin"
    COMMENT "Generating HEX/BIN firmware and reporting size"
    VERBATIM
)
