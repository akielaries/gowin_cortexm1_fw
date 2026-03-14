set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/itcm/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/itcm/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/itcm/lib)

# Linker script for the itcm (assuming it uses the same one for now)
set(LINKER_SCRIPT
    ${CMAKE_SOURCE_DIR}/bsp/startup/linker/GOWIN_M1_flash_burn.ld
)

# Bootloader source files
file(GLOB_RECURSE BOOTLOADER_SOURCES
    boot.c
    ${CMAKE_SOURCE_DIR}/src/debug.c
    ${CMAKE_SOURCE_DIR}/src/delay.c
    ${CMAKE_SOURCE_DIR}/src/gpio.c
    ${CMAKE_SOURCE_DIR}/src/hw.c
    ${CMAKE_SOURCE_DIR}/bsp/system/*.c
    ${CMAKE_SOURCE_DIR}/bsp/drivers/src/*.c
    ${CMAKE_SOURCE_DIR}/kernel/*.c
    ${CMAKE_SOURCE_DIR}/kernel/pendsv.S
)

file(GLOB STARTUP_SOURCES
    ${CMAKE_SOURCE_DIR}/bsp/startup/*.S
)

if(BOARD_VARIANT)
  set(TARGET_SUFFIX "_${BOARD_VARIANT}")
else()
  set(TARGET_SUFFIX "")
endif()

set(ITCM_ELF_TARGET "itcm${TARGET_SUFFIX}.elf")

# Define the itcm executable
add_executable(${ITCM_ELF_TARGET}
    ${STARTUP_SOURCES}
    ${BOOTLOADER_SOURCES}
)

# Include directories for the itcm
target_include_directories(${ITCM_ELF_TARGET} PRIVATE
    ${CMAKE_SOURCE_DIR}/kernel
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/bsp/core
    ${CMAKE_SOURCE_DIR}/bsp/system
    ${CMAKE_SOURCE_DIR}/bsp/drivers/inc
    ${CMAKE_SOURCE_DIR}/cheby
)

# CPU + Compile options (same as main project)
target_compile_options(${ITCM_ELF_TARGET} PRIVATE
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

# Linker options (same as main project)
target_link_options(${ITCM_ELF_TARGET} PRIVATE
    -mcpu=cortex-m1
    -mthumb

    -T ${LINKER_SCRIPT}

    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/itcm${TARGET_SUFFIX}.map
)

# Post-build: generate .bin
add_custom_command(TARGET ${ITCM_ELF_TARGET} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY}
            -O binary
            $<TARGET_FILE:${ITCM_ELF_TARGET}>
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/itcm${TARGET_SUFFIX}.bin
)

# Post-build: print size
add_custom_command(TARGET ${ITCM_ELF_TARGET} POST_BUILD
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/size_summary.sh
            $<TARGET_FILE:${ITCM_ELF_TARGET}>
)
