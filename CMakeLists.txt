cmake_minimum_required(VERSION 3.13)

# Include the main Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(stargazing-vr C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()


# 2. Define our main executable
add_executable(${PROJECT_NAME}
    main.c
)

# 1. Add the subdirectory for the SD card library's build system
# (This path assumes you cloned the repo into your project's root)
add_subdirectory(lib/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/FatFs_SPI build)

pico_add_extra_outputs(${PROJECT_NAME})

# 3. Link all the libraries
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    FatFs_SPI
    
)

pico_enable_stdio_uart(${PROJECT_NAME} 1)
pico_enable_stdio_usb(${PROJECT_NAME} 0)

# ------------------------

