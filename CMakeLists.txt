cmake_minimum_required(VERSION 3.13)

# Include the main Pico SDK
include($ENV{PICO_SDK_PATH}/picosdk_init.cmake)
project(my_sd_card_project C CXX ASM)

# Initialize the SDK
pico_sdk_init()

# 1. Add the subdirectory for the SD card library's build system
# (This path assumes you cloned the repo into your project's root)
add_subdirectory(lib/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/src)

# 2. Define our main executable
add_executable(my_project
    src/main.c
    
    # We also need to build the hardware config from the example
    src/hw_config.c 
)

# 3. Link all the libraries
target_link_libraries(my_project
    pico_stdlib
    
    # These are needed by the sd_card_pico library
    hardware_dma
    hardware_pio
    
    # Link the SD card library (named 'sd_card_pico')
    # and FatFS (named 'ff') - these names come from the library's CMake file
    sd_card_pico
    ff
)

# 4. We MUST add the include paths for our main.c
# This is what directly fixes the "f_util.h not found" error
target_include_directories(my_project PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/include
    ${CMAKE_CURRENT_LIST_DIR}/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/lib/FatFs/source
)

# 5. Add the PIO file for the SD card
pico_generate_pio_header(my_project
    no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/src/sdio.pio
)

# ------------------------

pico_add_extra_outputs(my_project)