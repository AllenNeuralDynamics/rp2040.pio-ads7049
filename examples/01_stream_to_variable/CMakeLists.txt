cmake_minimum_required(VERSION 3.13)

#add_definitions(-DDEBUG) # Uncomment for debugging

# PICO_SDK_PATH must be defined.
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

# Use modern conventions like std::invoke
set(CMAKE_CXX_STANDARD 17)

project(stream_to_variable)

pico_sdk_init()
add_subdirectory(../../ build) # Path to pio_ads7049 library's CMakeLists.txt

add_executable(${PROJECT_NAME}
    src/main.cpp
)

include_directories(inc)

target_link_libraries(${PROJECT_NAME} pico_stdlib pio_ads7049)

pico_add_extra_outputs(${PROJECT_NAME})

if(DEBUG)
    message(WARNING "Debug printf() messages from harp core to UART with baud \
            rate 921600.")
    pico_enable_stdio_uart(${PROJECT_NAME} 1) # UART stdio for printf.
    # Additional libraries need to have stdio init also.
endif()

