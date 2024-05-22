# @file      CMakeLists.txt
# @brief     mender-mcu-client esp-idf component CMakeLists file
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

# List of sources
list(APPEND srcs
    "${CMAKE_CURRENT_LIST_DIR}/../core/src/mender-api.c"
    "${CMAKE_CURRENT_LIST_DIR}/../core/src/mender-artifact.c"
    "${CMAKE_CURRENT_LIST_DIR}/../core/src/mender-client.c"
    "${CMAKE_CURRENT_LIST_DIR}/../core/src/mender-utils.c"
    "${CMAKE_CURRENT_LIST_DIR}/../platform/flash/${CONFIG_MENDER_PLATFORM_FLASH_TYPE}/src/mender-flash.c"
    "${CMAKE_CURRENT_LIST_DIR}/../platform/log/${CONFIG_MENDER_PLATFORM_LOG_TYPE}/src/mender-log.c"
    "${CMAKE_CURRENT_LIST_DIR}/../platform/net/${CONFIG_MENDER_PLATFORM_NET_TYPE}/src/mender-http.c"
    "${CMAKE_CURRENT_LIST_DIR}/../platform/scheduler/${CONFIG_MENDER_PLATFORM_SCHEDULER_TYPE}/src/mender-scheduler.c"
    "${CMAKE_CURRENT_LIST_DIR}/../platform/storage/${CONFIG_MENDER_PLATFORM_STORAGE_TYPE}/src/mender-storage.c"
    "${CMAKE_CURRENT_LIST_DIR}/../platform/tls/${CONFIG_MENDER_PLATFORM_TLS_TYPE}/src/mender-tls.c"
)
if(CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE)
    list(APPEND srcs
        "${CMAKE_CURRENT_LIST_DIR}/../add-ons/src/mender-configure.c"
    )
endif()
if(CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY)
    list(APPEND srcs
        "${CMAKE_CURRENT_LIST_DIR}/../add-ons/src/mender-inventory.c"
    )
endif()
if(CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT)
    list(APPEND srcs
        "${CMAKE_CURRENT_LIST_DIR}/../add-ons/src/mender-troubleshoot.c"
        "${CMAKE_CURRENT_LIST_DIR}/../platform/net/${CONFIG_MENDER_PLATFORM_NET_TYPE}/src/mender-websocket.c"
    )
endif()

# Register mender-mcu-client component
idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../include"
    REQUIRES app_update esp_http_client json mbedtls nvs_flash
)
if (CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT)
    idf_component_optional_requires(PRIVATE espressif__esp_websocket_client esp_event msgpack-c)
endif()

# Retrieve mender-mcu-client version
file (STRINGS "${CMAKE_CURRENT_LIST_DIR}/../VERSION" MENDER_CLIENT_VERSION)
add_definitions("-DMENDER_CLIENT_VERSION=\"${MENDER_CLIENT_VERSION}\"")
