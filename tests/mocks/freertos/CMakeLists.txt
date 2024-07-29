# @file      CMakeLists.txt
# @brief     FreeRTOS mock CMakeLists file
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

# Fetch repository
set(GIT_REPO_URL "https://github.com/FreeRTOS/FreeRTOS-Kernel")
set(GIT_TAG_NAME "V10.5.1")

# Fetch destination
set(GIT_FOLDER_NAME "${CMAKE_CURRENT_LIST_DIR}/freertos")

# Declare fetch content
include(FetchContent)
FetchContent_Declare(
    freertos
    URL                        "${GIT_REPO_URL}/archive/refs/tags/${GIT_TAG_NAME}.zip"
    SOURCE_DIR                 "${GIT_FOLDER_NAME}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# Fetch if not already done
FetchContent_GetProperties(freertos)
if(NOT freertos_POPULATED)
    message("Populating ${GIT_FOLDER_NAME}, please wait...")
    FetchContent_Populate(freertos)
endif()

# Add sources
target_sources(${EXECUTABLE_NAME} PRIVATE
    "${GIT_FOLDER_NAME}/croutine.c"
    "${GIT_FOLDER_NAME}/event_groups.c"
    "${GIT_FOLDER_NAME}/list.c"
    "${GIT_FOLDER_NAME}/queue.c"
    "${GIT_FOLDER_NAME}/stream_buffer.c"
    "${GIT_FOLDER_NAME}/tasks.c"
    "${GIT_FOLDER_NAME}/timers.c"
    "${GIT_FOLDER_NAME}/portable/MemMang/heap_3.c"
    "${GIT_FOLDER_NAME}/portable/ThirdParty/GCC/Posix/port.c"
    "${GIT_FOLDER_NAME}/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c"
)

# Add include directories
include_directories("${GIT_FOLDER_NAME}/include")
include_directories("${GIT_FOLDER_NAME}/portable/ThirdParty/GCC/Posix")
include_directories("${GIT_FOLDER_NAME}/portable/ThirdParty/GCC/Posix/utils")
include_directories("${CMAKE_CURRENT_LIST_DIR}/include")
