cmake_minimum_required(VERSION 3.16)

# Set the project name
project(softcore_fw_example C ASM)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)

# --------------------------------------------------
# Board variant
# --------------------------------------------------
set(BOARD_VARIANT "" CACHE STRING "Board variant: MEGA_138K or MEGA_60K")
if(BOARD_VARIANT)
  add_compile_definitions(${BOARD_VARIANT})
  set(TARGET_SUFFIX "_${BOARD_VARIANT}")
else()
  set(TARGET_SUFFIX "")
endif()

set(ELF_TARGET "${PROJECT_NAME}${TARGET_SUFFIX}.elf")

# --------------------------------------------------
# Output directories
# --------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# --------------------------------------------------
# Linker script
# --------------------------------------------------
set(LINKER_SCRIPT
    ${CMAKE_SOURCE_DIR}/bsp/startup/linker/GOWIN_M1_flash_burn.ld
)

# --------------------------------------------------
# Source files
# --------------------------------------------------
file(GLOB_RECURSE SOURCES
  #src/ethernet_demo.c
  #src/sfp_demo.c
  src/gpio_ffc_demo.c
  src/networking/*.c
  src/debug.c

  src/hw.c
  src/delay.c
  src/gpio.c

  bsp/system/*.c
  bsp/drivers/src/*.c
  kernel/*.c
  kernel/pendsv.S
)

file(GLOB STARTUP_SOURCES
    bsp/startup/*.S
)

# --------------------------------------------------
# Bootloader subdirectory
# --------------------------------------------------
add_subdirectory(itcm)

# --------------------------------------------------
# Executable
# --------------------------------------------------
add_executable(${ELF_TARGET}
    ${STARTUP_SOURCES}
    ${SOURCES}
)

# --------------------------------------------------
# Include directories
# --------------------------------------------------
target_include_directories(${ELF_TARGET} PRIVATE
    src
    src/networking
    bsp/core
    bsp/system
    bsp/drivers/inc
    kernel
    cheby
)


# --------------------------------------------------
# CPU + Compile options
# --------------------------------------------------
target_compile_options(${ELF_TARGET} PRIVATE
    -mcpu=cortex-m1
    -mthumb

    -O0
    -g3

    -fmessage-length=0
    -fsigned-char
    -ffunction-sections
    -fdata-sections

    -ffixed-r8
    -ffixed-r9
    -ffixed-r10
    -ffixed-r11

    -std=gnu11
)

# --------------------------------------------------
# Linker options
# --------------------------------------------------
target_link_options(${ELF_TARGET} PRIVATE
    -mcpu=cortex-m1
    -mthumb

    -T ${LINKER_SCRIPT}

    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}${TARGET_SUFFIX}.map
)

# --------------------------------------------------
# Post-build: generate .bin
# --------------------------------------------------
add_custom_command(TARGET ${ELF_TARGET} POST_BUILD
    COMMAND arm-none-eabi-objcopy
            -O binary
            $<TARGET_FILE:${ELF_TARGET}>
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}${TARGET_SUFFIX}.bin
)

# --------------------------------------------------
# Post-build: print size
# --------------------------------------------------
add_custom_command(TARGET ${ELF_TARGET} POST_BUILD
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/size_summary.sh
            $<TARGET_FILE:${ELF_TARGET}>
)
