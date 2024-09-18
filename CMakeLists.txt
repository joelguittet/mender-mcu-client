# @file      CMakeLists.txt
# @brief     mender-mcu-client static library CMakeLists file
#
# Copyright joelguittet and mender-mcu-client contributors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.16.3)

# Library project
project(mender-mcu-client LANGUAGES C)

# Creation of the library
add_library(mender-mcu-client STATIC)

# Library options
if (NOT CONFIG_MENDER_SERVER_HOST)
    message(STATUS "Using default server host")
else()
    message(STATUS "Using custom '${CONFIG_MENDER_SERVER_HOST}' server host")
endif()
if (NOT CONFIG_MENDER_SERVER_TENANT_TOKEN)
    message(STATUS "Using default server tenant token")
else()
    message(STATUS "Using custom '${CONFIG_MENDER_SERVER_TENANT_TOKEN}' server tenant token")
endif()
if (NOT CONFIG_MENDER_CLIENT_AUTHENTICATION_POLL_INTERVAL)
    message(STATUS "Using default authentication poll interval")
else()
    message(STATUS "Using custom '${CONFIG_MENDER_CLIENT_AUTHENTICATION_POLL_INTERVAL}' authentication poll interval")
endif()
if (NOT CONFIG_MENDER_CLIENT_UPDATE_POLL_INTERVAL)
    message(STATUS "Using default update poll interval")
else()
    message(STATUS "Using custom '${CONFIG_MENDER_CLIENT_UPDATE_POLL_INTERVAL}' update poll interval")
endif()
option(CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE "Mender client Configure" OFF)
option(CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE "Mender client Configure storage" ON)
if (CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE)
    message(STATUS "Using mender-configure add-on")
    if (CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE)
        message(STATUS "Using mender-configure storage")
    endif()
    if (NOT CONFIG_MENDER_CLIENT_CONFIGURE_REFRESH_INTERVAL)
        message(STATUS "Using default configure refresh interval")
    else()
        message(STATUS "Using custom '${CONFIG_MENDER_CLIENT_CONFIGURE_REFRESH_INTERVAL}' configure refresh interval")
    endif()
endif()
option(CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY "Mender client Inventory" ON)
if (CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY)
    message(STATUS "Using mender-inventory add-on")
    if (NOT CONFIG_MENDER_CLIENT_INVENTORY_REFRESH_INTERVAL)
        message(STATUS "Using default inventory refresh interval")
    else()
        message(STATUS "Using custom '${CONFIG_MENDER_CLIENT_INVENTORY_REFRESH_INTERVAL}' inventory refresh interval")
    endif()
endif()
option(CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT "Mender client Troubleshoot (EXPERIMENTAL)" OFF)
if (CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT)
    message(STATUS "Using mender-troubleshoot add-on")
    if (NOT CONFIG_MENDER_CLIENT_TROUBLESHOOT_HEALTHCHECK_INTERVAL)
        message(STATUS "Using default troubleshoot healthcheck interval")
    else()
        message(STATUS "Using custom '${CONFIG_MENDER_CLIENT_TROUBLESHOOT_HEALTHCHECK_INTERVAL}' troubleshoot healthcheck interval")
    endif()
    option(CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL "Mender client Troubleshoot Shell" ON)
    if (CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL)
        message(STATUS "Using mender-troubleshoot add-on shell feature")
    endif()
    option(CONFIG_MENDER_CLIENT_TROUBLESHOOT_FILE_TRANSFER "Mender client Troubleshoot File Transfer" OFF)
    if (CONFIG_MENDER_CLIENT_TROUBLESHOOT_FILE_TRANSFER)
        message(STATUS "Using mender-troubleshoot add-on file transfer feature")
    endif()
    option(CONFIG_MENDER_CLIENT_TROUBLESHOOT_PORT_FORWARDING "Mender client Troubleshoot Port Forwarding Transfer" OFF)
    if (CONFIG_MENDER_CLIENT_TROUBLESHOOT_PORT_FORWARDING)
        message(STATUS "Using mender-troubleshoot add-on port forwarding feature")
    endif()
endif()
if (NOT CONFIG_MENDER_LOG_LEVEL)
    message(STATUS "Using default log level")
elseif (CONFIG_MENDER_LOG_LEVEL STREQUAL "off")
    message(STATUS "Using log level 'off'")
    set(CONFIG_MENDER_LOG_LEVEL MENDER_LOG_LEVEL_OFF)
elseif (CONFIG_MENDER_LOG_LEVEL STREQUAL "error")
    message(STATUS "Using log level 'error'")
    set(CONFIG_MENDER_LOG_LEVEL MENDER_LOG_LEVEL_ERR)
elseif (CONFIG_MENDER_LOG_LEVEL STREQUAL "warning")
    message(STATUS "Using log level 'warning'")
    set(CONFIG_MENDER_LOG_LEVEL MENDER_LOG_LEVEL_WRN)
elseif (CONFIG_MENDER_LOG_LEVEL STREQUAL "info")
    message(STATUS "Using log level 'info'")
    set(CONFIG_MENDER_LOG_LEVEL MENDER_LOG_LEVEL_INF)
elseif (CONFIG_MENDER_LOG_LEVEL STREQUAL "debug")
    message(STATUS "Using log level 'debug'")
    set(CONFIG_MENDER_LOG_LEVEL MENDER_LOG_LEVEL_DBG)
else()
    message(FATAL_ERROR "Invalid log level '${CONFIG_MENDER_LOG_LEVEL}'")
endif()
if (NOT CONFIG_MENDER_PLATFORM_FLASH_TYPE)
    message(STATUS "Using default 'generic/weak' platform flash implementation")
    set(CONFIG_MENDER_PLATFORM_FLASH_TYPE "generic/weak")
else()
    message(STATUS "Using custom '${CONFIG_MENDER_PLATFORM_FLASH_TYPE}' platform flash implementation")
endif()
if (NOT CONFIG_MENDER_PLATFORM_LOG_TYPE)
    message(STATUS "Using default 'generic/weak' platform log implementation")
    set(CONFIG_MENDER_PLATFORM_LOG_TYPE "generic/weak")
else()
    message(STATUS "Using custom '${CONFIG_MENDER_PLATFORM_LOG_TYPE}' platform log implementation")
endif()
if (NOT CONFIG_MENDER_PLATFORM_NET_TYPE)
    message(STATUS "Using default 'generic/weak' platform network implementation")
    set(CONFIG_MENDER_PLATFORM_NET_TYPE "generic/weak")
else()
    message(STATUS "Using custom '${CONFIG_MENDER_PLATFORM_NET_TYPE}' platform network implementation")
endif()
if (NOT CONFIG_MENDER_PLATFORM_SCHEDULER_TYPE)
    message(STATUS "Using default 'generic/weak' platform scheduler implementation")
    set(CONFIG_MENDER_PLATFORM_SCHEDULER_TYPE "generic/weak")
else()
    message(STATUS "Using custom '${CONFIG_MENDER_PLATFORM_SCHEDULER_TYPE}' platform scheduler implementation")
endif()
if (NOT CONFIG_MENDER_PLATFORM_STORAGE_TYPE)
    message(STATUS "Using default 'generic/weak' platform storage implementation")
    set(CONFIG_MENDER_PLATFORM_STORAGE_TYPE "generic/weak")
else()
    message(STATUS "Using custom '${CONFIG_MENDER_PLATFORM_STORAGE_TYPE}' platform storage implementation")
endif()
if (NOT CONFIG_MENDER_PLATFORM_TLS_TYPE)
    message(STATUS "Using default 'generic/weak' platform TLS implementation")
    set(CONFIG_MENDER_PLATFORM_TLS_TYPE "generic/weak")
else()
    message(STATUS "Using custom '${CONFIG_MENDER_PLATFORM_TLS_TYPE}' platform TLS implementation")
endif()

# Definitions
if (CONFIG_MENDER_SERVER_HOST)
    target_compile_definitions(mender-mcu-client PRIVATE CONFIG_MENDER_SERVER_HOST=\"${CONFIG_MENDER_SERVER_HOST}\")
endif()
if (CONFIG_MENDER_SERVER_TENANT_TOKEN)
    target_compile_definitions(mender-mcu-client PRIVATE CONFIG_MENDER_SERVER_TENANT_TOKEN=\"${CONFIG_MENDER_SERVER_TENANT_TOKEN}\")
endif()
if (CONFIG_MENDER_CLIENT_AUTHENTICATION_POLL_INTERVAL)
    target_compile_definitions(mender-mcu-client PRIVATE CONFIG_MENDER_CLIENT_AUTHENTICATION_POLL_INTERVAL=${CONFIG_MENDER_CLIENT_AUTHENTICATION_POLL_INTERVAL})
endif()
if (CONFIG_MENDER_CLIENT_UPDATE_POLL_INTERVAL)
    target_compile_definitions(mender-mcu-client PRIVATE CONFIG_MENDER_CLIENT_UPDATE_POLL_INTERVAL=${CONFIG_MENDER_CLIENT_UPDATE_POLL_INTERVAL})
endif()
if (CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE)
    target_compile_definitions(mender-mcu-client PUBLIC CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE)
    if (CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE)
        target_compile_definitions(mender-mcu-client PUBLIC CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE)
    endif()
    if (CONFIG_MENDER_CLIENT_CONFIGURE_REFRESH_INTERVAL)
        target_compile_definitions(mender-mcu-client PRIVATE CONFIG_MENDER_CLIENT_CONFIGURE_REFRESH_INTERVAL=${CONFIG_MENDER_CLIENT_CONFIGURE_REFRESH_INTERVAL})
    endif()
endif()
if (CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY)
    target_compile_definitions(mender-mcu-client PUBLIC CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY)
    if (CONFIG_MENDER_CLIENT_INVENTORY_REFRESH_INTERVAL)
        target_compile_definitions(mender-mcu-client PRIVATE CONFIG_MENDER_CLIENT_INVENTORY_REFRESH_INTERVAL=${CONFIG_MENDER_CLIENT_INVENTORY_REFRESH_INTERVAL})
    endif()
endif()
if (CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT)
    target_compile_definitions(mender-mcu-client PUBLIC CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT)
    if (CONFIG_MENDER_CLIENT_TROUBLESHOOT_HEALTHCHECK_INTERVAL)
        target_compile_definitions(mender-mcu-client PRIVATE CONFIG_MENDER_CLIENT_TROUBLESHOOT_HEALTHCHECK_INTERVAL=${CONFIG_MENDER_CLIENT_TROUBLESHOOT_HEALTHCHECK_INTERVAL})
    endif()
    if (CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL)
        target_compile_definitions(mender-mcu-client PUBLIC CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL)
    endif()
    if (CONFIG_MENDER_CLIENT_TROUBLESHOOT_FILE_TRANSFER)
        target_compile_definitions(mender-mcu-client PUBLIC CONFIG_MENDER_CLIENT_TROUBLESHOOT_FILE_TRANSFER)
    endif()
    if (CONFIG_MENDER_CLIENT_TROUBLESHOOT_PORT_FORWARDING)
        target_compile_definitions(mender-mcu-client PUBLIC CONFIG_MENDER_CLIENT_TROUBLESHOOT_PORT_FORWARDING)
    endif()
endif()
if (CONFIG_MENDER_LOG_LEVEL)
    target_compile_definitions(mender-mcu-client PUBLIC CONFIG_MENDER_LOG_LEVEL=${CONFIG_MENDER_LOG_LEVEL})
endif()

# List of sources
file(GLOB SOURCES_TEMP
    "${CMAKE_CURRENT_LIST_DIR}/core/src/mender-api.c"
    "${CMAKE_CURRENT_LIST_DIR}/core/src/mender-artifact.c"
    "${CMAKE_CURRENT_LIST_DIR}/core/src/mender-client.c"
    "${CMAKE_CURRENT_LIST_DIR}/core/src/mender-utils.c"
    "${CMAKE_CURRENT_LIST_DIR}/platform/flash/${CONFIG_MENDER_PLATFORM_FLASH_TYPE}/src/mender-flash.c"
    "${CMAKE_CURRENT_LIST_DIR}/platform/log/${CONFIG_MENDER_PLATFORM_LOG_TYPE}/src/mender-log.c"
    "${CMAKE_CURRENT_LIST_DIR}/platform/net/${CONFIG_MENDER_PLATFORM_NET_TYPE}/src/mender-http.c"
    "${CMAKE_CURRENT_LIST_DIR}/platform/scheduler/${CONFIG_MENDER_PLATFORM_SCHEDULER_TYPE}/src/mender-scheduler.c"
    "${CMAKE_CURRENT_LIST_DIR}/platform/storage/${CONFIG_MENDER_PLATFORM_STORAGE_TYPE}/src/mender-storage.c"
    "${CMAKE_CURRENT_LIST_DIR}/platform/tls/${CONFIG_MENDER_PLATFORM_TLS_TYPE}/src/mender-tls.c"
)
if (CONFIG_MENDER_PLATFORM_NET_TYPE STREQUAL "zephyr")
    list(APPEND SOURCES_TEMP
        "${CMAKE_CURRENT_LIST_DIR}/platform/net/${CONFIG_MENDER_PLATFORM_NET_TYPE}/src/mender-net.c"
    )
endif()
if (CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE)
    list(APPEND SOURCES_TEMP
        "${CMAKE_CURRENT_LIST_DIR}/add-ons/configure/src/mender-configure.c"
        "${CMAKE_CURRENT_LIST_DIR}/add-ons/configure/src/mender-configure-api.c"
    )
endif()
if (CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY)
    list(APPEND SOURCES_TEMP
        "${CMAKE_CURRENT_LIST_DIR}/add-ons/inventory/src/mender-inventory.c"
        "${CMAKE_CURRENT_LIST_DIR}/add-ons/inventory/src/mender-inventory-api.c"
    )
endif()
if (CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT)
    list(APPEND SOURCES_TEMP
        "${CMAKE_CURRENT_LIST_DIR}/add-ons/troubleshoot/src/mender-troubleshoot.c"
        "${CMAKE_CURRENT_LIST_DIR}/add-ons/troubleshoot/src/mender-troubleshoot-api.c"
        "${CMAKE_CURRENT_LIST_DIR}/add-ons/troubleshoot/src/mender-troubleshoot-control.c"
        "${CMAKE_CURRENT_LIST_DIR}/add-ons/troubleshoot/src/mender-troubleshoot-mender-client.c"
        "${CMAKE_CURRENT_LIST_DIR}/add-ons/troubleshoot/src/mender-troubleshoot-msgpack.c"
        "${CMAKE_CURRENT_LIST_DIR}/add-ons/troubleshoot/src/mender-troubleshoot-protomsg.c"
        "${CMAKE_CURRENT_LIST_DIR}/platform/net/${CONFIG_MENDER_PLATFORM_NET_TYPE}/src/mender-websocket.c"
    )
    if (CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL)
        list(APPEND SOURCES_TEMP
            "${CMAKE_CURRENT_LIST_DIR}/add-ons/troubleshoot/src/mender-troubleshoot-shell.c"
        )
    endif()
    if (CONFIG_MENDER_CLIENT_TROUBLESHOOT_FILE_TRANSFER)
        list(APPEND SOURCES_TEMP
            "${CMAKE_CURRENT_LIST_DIR}/add-ons/troubleshoot/src/mender-troubleshoot-file-transfer.c"
        )
    endif()
    if (CONFIG_MENDER_CLIENT_TROUBLESHOOT_PORT_FORWARDING)
        list(APPEND SOURCES_TEMP
            "${CMAKE_CURRENT_LIST_DIR}/add-ons/troubleshoot/src/mender-troubleshoot-port-forwarding.c"
        )
    endif()
endif()
target_sources(mender-mcu-client PRIVATE ${SOURCES_TEMP})

# Add include directories
target_include_directories(mender-mcu-client PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
if (CONFIG_MENDER_PLATFORM_NET_TYPE STREQUAL "zephyr")
    target_include_directories(mender-mcu-client PRIVATE "${CMAKE_CURRENT_LIST_DIR}/platform/net/${CONFIG_MENDER_PLATFORM_NET_TYPE}/include")
endif()
if (CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE)
    target_include_directories(mender-mcu-client PUBLIC "${CMAKE_CURRENT_LIST_DIR}/add-ons/configure/include")
endif()
if (CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY)
    target_include_directories(mender-mcu-client PUBLIC "${CMAKE_CURRENT_LIST_DIR}/add-ons/inventory/include")
endif()
if (CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT)
    target_include_directories(mender-mcu-client PUBLIC "${CMAKE_CURRENT_LIST_DIR}/add-ons/troubleshoot/include")
endif()

# Define version
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/VERSION" MENDER_CLIENT_VERSION)
add_definitions("-DMENDER_CLIENT_VERSION=\"${MENDER_CLIENT_VERSION}\"")
