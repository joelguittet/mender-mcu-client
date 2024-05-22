# @file      CMakeLists.txt
# @brief     Test application CMakeLists file
#
# MIT License
#
# Copyright (c) 2022-2023 joelguittet and mender-mcu-client contributors
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.16.3)

# CMake configurations
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Configs" FORCE)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define PROJECT_BASE_NAME
set(PROJECT_BASE_NAME mender-mcu-client)
message("Configuring for ${PROJECT_BASE_NAME} - Build type is ${CMAKE_BUILD_TYPE}")

# Flags
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Og -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og -g")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
endif()

# Declare the executable first, so that we can add flags and sources later on
set(APP_EXECUTABLE_NAME ${PROJECT_BASE_NAME}.elf)
message("Executable name: ${APP_EXECUTABLE_NAME}")
add_executable(${APP_EXECUTABLE_NAME})

# Define compile options
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_definitions(${APP_EXECUTABLE_NAME} PRIVATE DEBUG)
endif()

# Add definitions to configure the client
if(CONFIG_MENDER_SERVER_HOST)
    add_compile_definitions(CONFIG_MENDER_SERVER_HOST="${CONFIG_MENDER_SERVER_HOST}")
endif()
if(CONFIG_MENDER_CLIENT_AUTHENTICATION_POLL_INTERVAL)
    add_compile_definitions(CONFIG_MENDER_CLIENT_AUTHENTICATION_POLL_INTERVAL=${CONFIG_MENDER_CLIENT_AUTHENTICATION_POLL_INTERVAL})
endif()
if(CONFIG_MENDER_CLIENT_UPDATE_POLL_INTERVAL)
    add_compile_definitions(CONFIG_MENDER_CLIENT_UPDATE_POLL_INTERVAL=${CONFIG_MENDER_CLIENT_UPDATE_POLL_INTERVAL})
endif()
if(CONFIG_MENDER_FLASH_PATH)
    add_compile_definitions(CONFIG_MENDER_FLASH_PATH="${CONFIG_MENDER_FLASH_PATH}")
endif()
if(CONFIG_MENDER_STORAGE_PATH)
    add_compile_definitions(CONFIG_MENDER_STORAGE_PATH="${CONFIG_MENDER_STORAGE_PATH}")
endif()
if(CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE)
    add_compile_definitions(CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE)
    if(CONFIG_MENDER_CLIENT_CONFIGURE_REFRESH_INTERVAL)
        add_compile_definitions(CONFIG_MENDER_CLIENT_CONFIGURE_REFRESH_INTERVAL=${CONFIG_MENDER_CLIENT_CONFIGURE_REFRESH_INTERVAL})
    endif()
    if(CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE)
        add_compile_definitions(CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE)
    endif()
endif()
if(CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY)
    add_compile_definitions(CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY)
    if(CONFIG_MENDER_CLIENT_INVENTORY_REFRESH_INTERVAL)
        add_compile_definitions(CONFIG_MENDER_CLIENT_INVENTORY_REFRESH_INTERVAL=${CONFIG_MENDER_CLIENT_INVENTORY_REFRESH_INTERVAL})
    endif()
endif()
if(CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT)
    add_compile_definitions(CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT)
endif()

# Add definitions depending of the target
if(CONFIG_MENDER_PLATFORM_NET_TYPE MATCHES "generic/curl")
    add_compile_definitions(CONFIG_MENDER_WEBSOCKET_THREAD_STACK_SIZE=64)
    add_compile_definitions(CONFIG_MENDER_WEBSOCKET_THREAD_PRIORITY=0)
endif()
if(CONFIG_MENDER_PLATFORM_NET_TYPE MATCHES "zephyr")
    add_compile_definitions(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
    add_compile_definitions(CONFIG_MENDER_NET_CA_CERTIFICATE_TAG=1)
endif()
if(CONFIG_MENDER_PLATFORM_SCHEDULER_TYPE MATCHES "freertos")
    add_compile_definitions(CONFIG_MENDER_SCHEDULER_WORK_QUEUE_STACK_SIZE=20)
    add_compile_definitions(CONFIG_MENDER_SCHEDULER_WORK_QUEUE_PRIORITY=5)
    add_compile_definitions(CONFIG_MENDER_SCHEDULER_WORK_QUEUE_LENGTH=10)
endif()
if(CONFIG_MENDER_PLATFORM_SCHEDULER_TYPE MATCHES "posix")
    add_compile_definitions(CONFIG_MENDER_SCHEDULER_WORK_QUEUE_STACK_SIZE=64)
    add_compile_definitions(CONFIG_MENDER_SCHEDULER_WORK_QUEUE_PRIORITY=0)
    add_compile_definitions(CONFIG_MENDER_SCHEDULER_WORK_QUEUE_LENGTH=10)
endif()
if(CONFIG_MENDER_PLATFORM_SCHEDULER_TYPE MATCHES "zephyr")
    add_compile_definitions(CONFIG_MENDER_SCHEDULER_WORK_QUEUE_STACK_SIZE=12)
    add_compile_definitions(CONFIG_MENDER_SCHEDULER_WORK_QUEUE_PRIORITY=5)
endif()
if(CONFIG_MENDER_PLATFORM_STORAGE_TYPE MATCHES "zephyr/nvs")
    add_compile_definitions(CONFIG_MENDER_STORAGE_NVS_SECTOR_COUNT=4)
endif()

# Add sources
file(GLOB_RECURSE SOURCES_TEMP "${CMAKE_CURRENT_LIST_DIR}/src/*.c")
target_sources(${APP_EXECUTABLE_NAME} PRIVATE ${SOURCES_TEMP})

# Include mocks
include("${CMAKE_CURRENT_LIST_DIR}/mocks/CMakeLists.txt")

# Include mender-mcu-client library
include_directories("${CMAKE_CURRENT_LIST_DIR}/../include")
include("${CMAKE_CURRENT_LIST_DIR}/../CMakeLists.txt")

# Link the executable with the mender-mcu-library
target_link_libraries(${APP_EXECUTABLE_NAME} mender-mcu-client pthread)
if(CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT)
    target_link_libraries(${APP_EXECUTABLE_NAME} msgpack-c)
endif()
if(CONFIG_MENDER_PLATFORM_NET_TYPE MATCHES "generic/curl")
    target_link_libraries(${APP_EXECUTABLE_NAME} curl)
endif()
if(CONFIG_MENDER_PLATFORM_SCHEDULER_TYPE MATCHES "posix")
    target_link_libraries(${APP_EXECUTABLE_NAME} rt)
endif()
if(CONFIG_MENDER_PLATFORM_TLS_TYPE MATCHES "generic/cryptoauthlib")
    target_link_libraries(${APP_EXECUTABLE_NAME} cryptoauth)
endif()

# Define project LANGUAGES
project(${PROJECT_BASE_NAME} LANGUAGES C ASM)

# Define project VERSION
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/../VERSION" VERSION_NUMBER LIMIT_COUNT 1)
STRING(REGEX REPLACE "^([0-9]+)\\.([0-9]+)\\.([0-9]+)-rc[0-9]+" "\\1.\\2.\\3" VERSION_NUMBER "${VERSION_NUMBER}")
project(${PROJECT_BASE_NAME} VERSION ${VERSION_NUMBER} LANGUAGES)
