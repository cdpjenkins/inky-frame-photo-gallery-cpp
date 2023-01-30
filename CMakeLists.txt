cmake_minimum_required(VERSION 3.12)

set(NAME inky-frame-photo-gallery-cpp)

include(pimoroni_pico_import.cmake)
include(pico_sdk_import.cmake)

# Gooey boilerplate
project(${NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

# Add your source files
add_executable(${NAME}
    main.cpp 
)

# Include required libraries
# This assumes `pimoroni-pico` is stored alongside your project
include(common/pimoroni_i2c)
include(common/pimoroni_bus)
include(libraries/bitmap_fonts/bitmap_fonts)
include(libraries/hershey_fonts/hershey_fonts)
include(libraries/pico_explorer/pico_explorer)
include(libraries/inky_frame/inky_frame)
include(libraries/jpegdec/jpegdec)
include(drivers/fatfs/fatfs)
include(drivers/sdcard/sdcard)
include(drivers/pcf85063a/pcf85063a)
include(drivers/uc8159/uc8159)

# # Don't forget to link the libraries you need!
# target_link_libraries(${NAME}
#     # inky_frame INTERFACE
#     pimoroni_i2c
#     pico_explorer
#     hardware_i2c pico_graphics hardware_spi hardware_pwm bitmap_fonts hershey_fonts pico_stdlib # sdcard fatfs pcf85063a uc8159 jpegdec
# )

# target_link_libraries(${NAME} pico_stdlib jpegdec inky_frame fatfs hardware_pwm hardware_spi hardware_i2c hardware_rtc fatfs sdcard pico_graphics)
target_link_libraries(${NAME} pico_stdlib   hardware_pwm hardware_spi hardware_i2c hardware_rtc   pico_graphics inky_frame )


# create map/bin/hex file etc.
pico_add_extra_outputs(${NAME})

# Set up files for the release packages
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.uf2
    ${CMAKE_CURRENT_LIST_DIR}/README.md
    DESTINATION .
)

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_GENERATOR "ZIP" "TGZ")
include(CPack)
