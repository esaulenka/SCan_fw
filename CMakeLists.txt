cmake_minimum_required(VERSION 3.21)


option(SCAN_BOOTLOADER  "Build bootloader" OFF)
# check target_compile_definitions for possible values
set(SCAN_BOARD "BOARD_SIGMA" CACHE STRING "Target hardware")


# force using ARM GCC toolchain
# if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
#   set(CMAKE_TOOLCHAIN_FILE "toolchain-arm-none-eabi.cmake" CACHE PATH "toolchain file")
# endif()

# force using Release build, if other is not specified
if( x${CMAKE_BUILD_TYPE} STREQUAL x)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Use release build by default" FORCE)
endif()

# declare project itself
if (DEFINED CACHE{PRESET_NAME})
  project(${PRESET_NAME})
elseif (NOT SCAN_BOOTLOADER)
  project(SCan)
else()
  project(SCan-boot)
endif()
set(EXECUTABLE ${PROJECT_NAME}.elf)

set(SRC_COMMON
        system/startup.c
        system/vectable.c
        system/sysinit.cpp

        Rtt/SEGGER_RTT.c
        Rtt/SEGGER_RTT_printf.c

        USB/usb_control.c
        USB/usb_dwc_common.c
        USB/usb_f107.c
        USB/usb_standard.c
        USB/usb.c
)
set(SRC_BOOT 
        USB/dfu_descriptors.c
        boot_main.cpp
)
set(SRC_HACKER
        system/syscalls.c

        Can/candrv.cpp
        Lin/LinBus.cpp
        Lin/LinDrv.cpp

        USB/cdcacm.cpp

        main.cpp
        canhacker.cpp
        CanHackerBinary.cpp

        timer.cpp
)

set(INC_DIRECTORIES
        ${CMAKE_SOURCE_DIR}/.
        Rtt
        stm32tpl
        system
        system/CMSIS
        USB
)

enable_language(C CXX ASM)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

if(NOT SCAN_BOOTLOADER)
    add_executable(${EXECUTABLE} ${SRC_COMMON} ${SRC_HACKER})
else()
    add_executable(${EXECUTABLE} ${SRC_COMMON} ${SRC_BOOT})
endif()

target_compile_definitions(${EXECUTABLE} PRIVATE
        STM32F10X_CL
        STM32F105xC
        BOARD=${SCAN_BOARD}
        BOARD_SIGMA=1           # Sigma 10 module
        BOARD_2CAN=2            # Ancient 2CAN board
        BOARD_CSAT=3            # Custom CS device
        BOARD_2CAN30=4          # 2CAN 30 module
        BOARD_2CAN2LIN=5        # 2CAN-2LIN board

        PROTOCOL_LAWICEL=1      # commonly used, ascii-based
        PROTOCOL_BINARY=2       # "new" canhacker protocol
        PROTOCOL=2              # current protocol

        $<$<CONFIG:Debug>:DEBUG>
)

target_include_directories(${EXECUTABLE} PRIVATE ${INC_DIRECTORIES})

target_compile_options(${EXECUTABLE} PRIVATE
        -mcpu=cortex-m3
        -mfloat-abi=soft
        -specs=nosys.specs
        -fdata-sections
        -ffunction-sections
        -Wall
        $<$<CONFIG:Debug>:-Og>
        $<$<CONFIG:Release>:-O3>
        $<$<COMPILE_LANGUAGE:CXX>:
                -Wno-volatile           # very annoying in latest GCC 
                -fno-use-cxa-atexit
                -fno-exceptions
                -fno-rtti
        >
)

# use different linker scripts for core and bootloader
if(NOT SCAN_BOOTLOADER)
  set(LINKER_SCRIPT_NAME stm32F105xC)
else()
  set(LINKER_SCRIPT_NAME stm32F105xC_boot)
endif()
find_file(LINKER_SCRIPT 
        REQUIRED  NO_CMAKE_FIND_ROOT_PATH NO_CACHE
        NAMES ${LINKER_SCRIPT_NAME}.ld
        HINTS system)
target_link_directories(${EXECUTABLE} PRIVATE system)

target_link_options(${EXECUTABLE} PRIVATE
        -mcpu=cortex-m3 -mfloat-abi=soft
        -T${LINKER_SCRIPT}
        -nostartfiles
        -specs=nano.specs
        -specs=nosys.specs
        -Wl,-Map=${PROJECT_NAME}.map,--cref
        -Wl,--gc-sections
        $<$<CONFIG:Release>:-flto>
)

# Print executable size
add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
        COMMAND ${CMAKE_SIZE} ${EXECUTABLE} -d )

# Create hex file
add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex ${EXECUTABLE} ${PROJECT_NAME}.hex
        COMMENT "Creating HEX file ${PROJECT_NAME}.hex"
)
# Create DFU image
if(NOT SCAN_BOOTLOADER)
    add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
        COMMAND python ${CMAKE_SOURCE_DIR}/dfu-convert.py -i
                ${PROJECT_NAME}.hex ${PROJECT_NAME}.dfu
        COMMENT "Making DFU image ${PROJECT_NAME}.dfu"
    )
endif()
