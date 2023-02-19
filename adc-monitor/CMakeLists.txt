cmake_minimum_required(VERSION 3.10)
project(adc_monitor C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
    add_definitions(-DDEBUG_CONSOLE=1)
else ()
    add_definitions(-DDEBUG_CONSOLE=0)
endif ()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")

# add extra include directories
include_directories(inc)

# Build number --->
include_directories(${PROJECT_BINARY_DIR})

# Set variable for the generate sources
set(GENERATE_SRCS ${PROJECT_SOURCE_DIR}/src/utils/build_number.c)
set(GENERATE_OUTPUT ${PROJECT_BINARY_DIR}/build_number.o)
set(VERSION_OUTPUT ${PROJECT_BINARY_DIR}/build_number.h)
set(VERSION_MAJOR 1)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)
set(VERSION_META "") # beta/rc

# Create command to compile the generate command
add_custom_command(
        OUTPUT ${GENERATE_OUTPUT}
        COMMAND gcc -o ${GENERATE_OUTPUT} ${GENERATE_SRCS}
        DEPENDS ${GENERATE_SRCS}
)

add_custom_target(build_number
        COMMAND ${GENERATE_OUTPUT}
        -M ${VERSION_MAJOR}
        -m ${VERSION_MINOR}
        -p ${VERSION_PATCH}
        -a "${VERSION_META}"
        -F "${PROJECT_BINARY_DIR}/build_number"
        -P "ADC_MONITOR"
        -i
        > ${VERSION_OUTPUT}
        DEPENDS ${GENERATE_OUTPUT}
        VERBATIM
        )
# <---

add_executable(adc_monitor main.c
        inc/config.h
        inc/drivers/adc_mcp3008.h
        inc/drivers/gpio.h
        inc/tasks/api.h
        inc/tasks/mainloop.h
        inc/utils/libjson.h
        inc/utils/log.h
        inc/utils/sched_attr.h
        inc/utils/stdfun.h
        src/drivers/adc_mcp3008.c
        src/tasks/api.c
        src/tasks/mainloop.c
        src/utils/libjson.c
        src/utils/log.c
        src/utils/stdfun.c
        src/drivers/gpio.c
        )

# Add libraries
target_link_libraries(adc_monitor m) # to include math.h

# Add buildnumber dependency
add_dependencies(adc_monitor build_number)

# Post build command -->
set(OUTPUT_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
if (NOT ${VERSION_META} STREQUAL "")
    message("-- found version meta")
    set(OUTPUT_VERSION "${OUTPUT_VERSION}-${VERSION_META}")
endif ()
add_custom_command(TARGET adc_monitor
        POST_BUILD
        COMMAND /bin/sh ${PROJECT_SOURCE_DIR}/post_build.sh ${CMAKE_BUILD_TYPE} ${OUTPUT_VERSION}
        )
# <---