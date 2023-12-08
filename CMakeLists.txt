cmake_minimum_required(VERSION 3.13)

#add_definitions(-DDEBUG)

if(NOT DEFINED PICO_SDK_PATH)
    message(FATAL_ERROR
            "PICO_SDK_PATH was not specified.
             pico_sdk_init() must first be invoked.")
endif()

project(rp2040.pio-ads7049)

# Use modern conventions like std::invoke
set(CMAKE_CXX_STANDARD 17)

add_library(pio_ads7049
    src/pio_ads7049.cpp
)

pico_generate_pio_header(pio_ads7049
    ${CMAKE_CURRENT_LIST_DIR}/src/pio_ads7049.pio
)

# Header file location exposed with target scope for external projects.
target_include_directories(pio_ads7049 PUBLIC inc)

target_link_libraries(pio_ads7049 hardware_pio hardware_dma pico_stdlib)

if(DEBUG)
    # UART stdio for printf.
    pico_enable_stdio_uart(pio_ads7049 1)
endif()
