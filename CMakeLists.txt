 # ================================================================
 # Author   : German Mundinger
 # Date     : 2020
 # ================================================================

# cmake -DCMAKE_BUILD_TYPE=Debug ..

cmake_minimum_required(VERSION 3.22)

project(avr_node LANGUAGES C ASM VERSION 1.0.0.0)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)

FetchContent_Declare(
    common_code
    SOURCE_DIR      ${PROJECT_SOURCE_DIR}/external/common_code
    GIT_REPOSITORY  https://github.com/germandevelop/common.git
    GIT_TAG         main
)
FetchContent_GetProperties(common_code)
if(NOT common_code_POPULATED)
    FetchContent_Populate(common_code)
endif()

FetchContent_Declare(
    w5500_driver
    SOURCE_DIR      ${PROJECT_SOURCE_DIR}/external/w5500_driver
    GIT_REPOSITORY  https://github.com/Wiznet/ioLibrary_Driver.git
    GIT_TAG         v3.1.3
)
FetchContent_GetProperties(w5500_driver)
if(NOT w5500_driver_POPULATED)
    FetchContent_Populate(w5500_driver)
endif()

FetchContent_Declare(
    bmp280_driver
    SOURCE_DIR      ${PROJECT_SOURCE_DIR}/external/bmp280_driver
    GIT_REPOSITORY  https://github.com/boschsensortec/BMP2-Sensor-API.git
    GIT_TAG         v1.0.1
)
FetchContent_GetProperties(bmp280_driver)
if(NOT bmp280_driver_POPULATED)
    FetchContent_Populate(bmp280_driver)
endif()

FetchContent_Declare(
    lwjson_parser
    SOURCE_DIR      ${PROJECT_SOURCE_DIR}/external/lwjson_parser
    GIT_REPOSITORY  https://github.com/MaJerle/lwjson.git
    GIT_TAG         v1.6.1
)
FetchContent_GetProperties(lwjson_parser)
if(NOT lwjson_parser_POPULATED)
    FetchContent_Populate(lwjson_parser)
endif()


set(AVR_MCU atmega328p)
set(AVR_CPU_FREQUENCY 16000000UL)
#set(AVR_CPU_FREQUENCY 8000000UL)
set(AVR_PROGRAMMER usbasp)

set(LOW_FUSE 0xFF)	# 16Hz external oscillator
#set(LOW_FUSE 0xE2)	# 8Hz internal
set(HIGH_FUSE 0xD9)
set(EXTENDED_FUSE 0xFF)
#set(LOCK_BIT 0xFF)

find_program(AVR_CC avr-gcc REQUIRED)
find_program(AVR_OBJCOPY avr-objcopy REQUIRED)
find_program(AVR_SIZE avr-size REQUIRED)
find_program(AVR_STRIP avr-strip REQUIRED)
find_program(AVR_OBJDUMP avr-objdump REQUIRED)
find_program(AVR_UPLOADTOOL avrdude REQUIRED)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)
set(CMAKE_C_COMPILER ${AVR_CC})

add_subdirectory(external/common_code)

# Create one target
add_executable(avr_firmware src/main.c)
target_link_libraries(avr_firmware PRIVATE node)
target_link_libraries(avr_firmware PRIVATE std_error)
target_include_directories(avr_firmware
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        external/w5500_driver/Ethernet
        external/lwjson_parser/lwjson/src/include
        external/bmp280_driver
)
target_sources(avr_firmware
    PRIVATE
        src/mcu/config.h
        src/mcu/pcint_d.h
        src/mcu/pcint_d.c
        src/mcu/int_0.h
        src/mcu/int_0.c
        src/mcu/int_1.h
        src/mcu/int_1.c
        src/mcu/timer_1.h
        src/mcu/timer_1.c
        src/mcu/adc.h
        src/mcu/adc.c
        src/mcu/i2c.h
        src/mcu/i2c.c
        src/mcu/spi.h
        src/mcu/spi.c
        src/mcu/uart.h
        src/mcu/uart.c

        src/board.h
        src/board.c
        src/logger.h
        src/logger.c
        src/tcp_client.h
        src/tcp_client.c
        src/node.msg.h
        src/node.mapper.h
        src/node.mapper.c

        src/board_b02.h
        src/board_b02.c

        src/device/bmp280_sensor.h
        src/device/bmp280_sensor.c

        src/lwjson_opts.h

        external/lwjson_parser/lwjson/src/include/lwjson/lwjson_opt.h
        external/lwjson_parser/lwjson/src/include/lwjson/lwjson.h
        external/lwjson_parser/lwjson/src/lwjson/lwjson.c

        external/w5500_driver/Ethernet/wizchip_conf.h
        external/w5500_driver/Ethernet/wizchip_conf.c
        external/w5500_driver/Ethernet/socket.h
        external/w5500_driver/Ethernet/socket.c
        external/w5500_driver/Ethernet/W5500/w5500.h
        external/w5500_driver/Ethernet/W5500/w5500.c

        external/bmp280_driver/bmp2_defs.h
        external/bmp280_driver/bmp2.h
        external/bmp280_driver/bmp2.c
)

set_source_files_properties(external/w5500_driver/Ethernet/socket.c
    PROPERTIES COMPILE_OPTIONS "-Wno-parentheses")
set_source_files_properties(external/w5500_driver/Ethernet/wizchip_conf.c
    PROPERTIES COMPILE_OPTIONS "-Wno-missing-braces;-Wno-unused-parameter")

target_compile_definitions(avr_firmware
    PRIVATE
        -DF_CPU=${AVR_CPU_FREQUENCY}
        _WIZCHIP_=5500
        BMP2_DOUBLE_COMPENSATION
        #$<$<CONFIG:Debug>:__ASSERT_USE_STDERR> # Requires too much memory =(
        $<$<CONFIG:Release>:NDEBUG>
)
target_compile_features(avr_firmware
    PUBLIC
        c_std_17
)
target_compile_options(avr_firmware
    PUBLIC
        -mmcu=${AVR_MCU}
        -Wall
        -Wextra
        -pedantic
        $<$<CONFIG:Debug>:-Os>
        $<$<CONFIG:Debug>:-g0>
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:Release>:-g0>
)
target_link_options(avr_firmware
    PRIVATE
        -mmcu=${AVR_MCU}
)
set_target_properties(avr_firmware
    PROPERTIES
        C_STANDARD_REQUIRED ON
)

add_custom_command(TARGET avr_firmware
    POST_BUILD
        COMMAND ${AVR_OBJCOPY} --only-section=.text --only-section=.data --output-target=ihex avr_firmware avr_firmware.hex
)

add_custom_command(TARGET avr_firmware
    POST_BUILD
        COMMAND ${AVR_SIZE} --format=SysV --mcu=${AVR_MCU} avr_firmware
        COMMAND ${AVR_SIZE} --format=SysV --mcu=${AVR_MCU} avr_firmware.hex
)

add_custom_target(flash
    ${AVR_UPLOADTOOL}
        -c ${AVR_PROGRAMMER}
        -p ${AVR_MCU}
        -U flash:w:avr_firmware.hex:i
)

add_custom_target(fuse
    ${AVR_UPLOADTOOL}
        -c ${AVR_PROGRAMMER}
        -p ${AVR_MCU}
        -U lfuse:w:${LOW_FUSE}:m
        -U hfuse:w:${HIGH_FUSE}:m
        -U efuse:w:${EXTENDED_FUSE}:m
)

add_custom_target(info
    ${AVR_UPLOADTOOL}
        -c ${AVR_PROGRAMMER}
        -p ${AVR_MCU}
        -v
)
