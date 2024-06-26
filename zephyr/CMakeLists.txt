# @file      CMakeLists.txt
# @brief     mender-mcu-client zephyr module CMakeLists file
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

if(CONFIG_MENDER_MCU_CLIENT)
    zephyr_library()
    zephyr_library_sources(
        "${CMAKE_CURRENT_LIST_DIR}/../core/src/mender-api.c"
        "${CMAKE_CURRENT_LIST_DIR}/../core/src/mender-artifact.c"
        "${CMAKE_CURRENT_LIST_DIR}/../core/src/mender-client.c"
        "${CMAKE_CURRENT_LIST_DIR}/../core/src/mender-utils.c"
        "${CMAKE_CURRENT_LIST_DIR}/../platform/flash/${CONFIG_MENDER_PLATFORM_FLASH_TYPE}/src/mender-flash.c"
        "${CMAKE_CURRENT_LIST_DIR}/../platform/log/${CONFIG_MENDER_PLATFORM_LOG_TYPE}/src/mender-log.c"
        "${CMAKE_CURRENT_LIST_DIR}/../platform/net/${CONFIG_MENDER_PLATFORM_NET_TYPE}/src/mender-http.c"
        "${CMAKE_CURRENT_LIST_DIR}/../platform/net/${CONFIG_MENDER_PLATFORM_NET_TYPE}/src/mender-net.c"
        "${CMAKE_CURRENT_LIST_DIR}/../platform/scheduler/${CONFIG_MENDER_PLATFORM_SCHEDULER_TYPE}/src/mender-scheduler.c"
        "${CMAKE_CURRENT_LIST_DIR}/../platform/storage/${CONFIG_MENDER_PLATFORM_STORAGE_TYPE}/src/mender-storage.c"
        "${CMAKE_CURRENT_LIST_DIR}/../platform/tls/${CONFIG_MENDER_PLATFORM_TLS_TYPE}/src/mender-tls.c"
    )
    zephyr_library_sources_ifdef(CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE
        "${CMAKE_CURRENT_LIST_DIR}/../add-ons/src/mender-configure.c"
    )
    zephyr_library_sources_ifdef(CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY
        "${CMAKE_CURRENT_LIST_DIR}/../add-ons/src/mender-inventory.c"
    )
    zephyr_library_sources_ifdef(CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT
        "${CMAKE_CURRENT_LIST_DIR}/../add-ons/src/mender-troubleshoot.c"
        "${CMAKE_CURRENT_LIST_DIR}/../platform/net/${CONFIG_MENDER_PLATFORM_NET_TYPE}/src/mender-websocket.c"
        "${CMAKE_CURRENT_LIST_DIR}/../platform/shell/zephyr/src/mender-shell.c"
    )
    zephyr_include_directories("${CMAKE_CURRENT_LIST_DIR}/../include")
    zephyr_include_directories("${CMAKE_CURRENT_LIST_DIR}/../platform/net/${CONFIG_MENDER_PLATFORM_NET_TYPE}/include")
    file (STRINGS "${CMAKE_CURRENT_LIST_DIR}/../VERSION" MENDER_CLIENT_VERSION)
    zephyr_library_compile_definitions(-DMENDER_CLIENT_VERSION=\"${MENDER_CLIENT_VERSION}\")
    zephyr_library_compile_definitions(-D_POSIX_C_SOURCE=200809L)  # Required for strdup and strtok_r support
    zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
endif()
