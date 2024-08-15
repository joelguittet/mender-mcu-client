# @file      CMakeLists.txt
# @brief     mender-mcu-client esp-idf component CMakeLists file
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
        "${CMAKE_CURRENT_LIST_DIR}/../add-ons/configure/src/mender-configure.c"
        "${CMAKE_CURRENT_LIST_DIR}/../add-ons/configure/src/mender-configure-api.c"
    )
endif()
if(CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY)
    list(APPEND srcs
        "${CMAKE_CURRENT_LIST_DIR}/../add-ons/inventory/src/mender-inventory.c"
        "${CMAKE_CURRENT_LIST_DIR}/../add-ons/inventory/src/mender-inventory-api.c"
    )
endif()
if(CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT)
    list(APPEND srcs
        "${CMAKE_CURRENT_LIST_DIR}/../add-ons/troubleshoot/src/mender-troubleshoot.c"
        "${CMAKE_CURRENT_LIST_DIR}/../add-ons/troubleshoot/src/mender-troubleshoot-api.c"
        "${CMAKE_CURRENT_LIST_DIR}/../add-ons/troubleshoot/src/mender-troubleshoot-control.c"
        "${CMAKE_CURRENT_LIST_DIR}/../add-ons/troubleshoot/src/mender-troubleshoot-mender-client.c"
        "${CMAKE_CURRENT_LIST_DIR}/../add-ons/troubleshoot/src/mender-troubleshoot-msgpack.c"
        "${CMAKE_CURRENT_LIST_DIR}/../add-ons/troubleshoot/src/mender-troubleshoot-protomsg.c"
        "${CMAKE_CURRENT_LIST_DIR}/../platform/net/${CONFIG_MENDER_PLATFORM_NET_TYPE}/src/mender-websocket.c"
    )
    if(CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL)
        list(APPEND srcs
            "${CMAKE_CURRENT_LIST_DIR}/../add-ons/troubleshoot/src/mender-troubleshoot-shell.c"
        )
    endif()
    if(CONFIG_MENDER_CLIENT_TROUBLESHOOT_FILE_TRANSFER)
        list(APPEND srcs
            "${CMAKE_CURRENT_LIST_DIR}/../add-ons/troubleshoot/src/mender-troubleshoot-file-transfer.c"
        )
    endif()
endif()

# List of includes
list(APPEND incs
    "${CMAKE_CURRENT_LIST_DIR}/../include"
)
if(CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE)
    list(APPEND incs
        "${CMAKE_CURRENT_LIST_DIR}/../add-ons/configure/include"
    )
endif()
if(CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY)
    list(APPEND incs
        "${CMAKE_CURRENT_LIST_DIR}/../add-ons/inventory/include"
    )
endif()
if(CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT)
    list(APPEND incs
        "${CMAKE_CURRENT_LIST_DIR}/../add-ons/troubleshoot/include"
    )
endif()

# Register mender-mcu-client component
idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS ${incs}
    REQUIRES app_update esp_http_client json mbedtls nvs_flash
)
if (CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT)
    idf_component_optional_requires(PRIVATE espressif__esp_websocket_client esp_event msgpack-c)
endif()

# Retrieve mender-mcu-client version
file (STRINGS "${CMAKE_CURRENT_LIST_DIR}/../VERSION" MENDER_CLIENT_VERSION)
add_definitions("-DMENDER_CLIENT_VERSION=\"${MENDER_CLIENT_VERSION}\"")
