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

# Define VERSION_NUMBER
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/../VERSION" VERSION_NUMBER LIMIT_COUNT 1)
set_property(DIRECTORY . APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/../VERSION)
STRING(REGEX REPLACE "^([0-9]+)\\.([0-9]+)\\.([0-9]+)-rc[0-9]+" "\\1.\\2.\\3" VERSION_NUMBER "${VERSION_NUMBER}")

# Define CMAKE_PROJECT_NAME, CMAKE_PROJECT_VERSION and LANGUAGES
project(${PROJECT_BASE_NAME} VERSION ${VERSION_NUMBER} LANGUAGES C ASM)

# Declare the executable first, so that we can add flags and sources later on
set(EXECUTABLE_NAME ${PROJECT_BASE_NAME}.elf)
message("Executable name: ${EXECUTABLE_NAME}")
add_executable(${EXECUTABLE_NAME})

# Define compile options
if (CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(${EXECUTABLE_NAME} PRIVATE -O1 -g)
    target_compile_definitions(${EXECUTABLE_NAME} PRIVATE DEBUG)
else()
    target_compile_options(${EXECUTABLE_NAME} PRIVATE -Os)
endif()

# Add sources
file(GLOB_RECURSE SOURCES_TEMP "${CMAKE_CURRENT_LIST_DIR}/src/*.c")
target_sources(${EXECUTABLE_NAME} PRIVATE ${SOURCES_TEMP})

# Include mocks
include("${CMAKE_CURRENT_LIST_DIR}/mocks/CMakeLists.txt")

# Include mender-mcu-client library
include("${CMAKE_CURRENT_LIST_DIR}/../CMakeLists.txt")

# Add definitions to configure the client
if(CONFIG_MENDER_PLATFORM_NET_TYPE MATCHES "zephyr")
    target_compile_definitions(mender-mcu-client PRIVATE CONFIG_NET_SOCKETS_SOCKOPT_TLS)
    target_compile_definitions(mender-mcu-client PRIVATE CONFIG_MENDER_NET_CA_CERTIFICATE_TAG=1)
endif()
if(CONFIG_MENDER_PLATFORM_STORAGE_TYPE MATCHES "zephyr/nvs")
    target_compile_definitions(mender-mcu-client PRIVATE CONFIG_MENDER_STORAGE_NVS_SECTOR_COUNT=4)
endif()

# Link the mender-mcu-client library
if(CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT)
    target_link_libraries(mender-mcu-client msgpack-c)
endif()
if(CONFIG_MENDER_PLATFORM_NET_TYPE MATCHES "generic/curl")
    target_link_libraries(mender-mcu-client curl)
endif()
if(CONFIG_MENDER_PLATFORM_SCHEDULER_TYPE MATCHES "posix")
    target_link_libraries(mender-mcu-client rt)
endif()
if(CONFIG_MENDER_PLATFORM_TLS_TYPE MATCHES "generic/cryptoauthlib")
    target_link_libraries(mender-mcu-client cryptoauth)
endif()

# Link the executable with the mender-mcu-client library
target_link_libraries(${EXECUTABLE_NAME} mender-mcu-client pthread)
