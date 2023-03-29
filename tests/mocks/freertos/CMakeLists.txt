# @file      CMakeLists.txt
# @brief     FreeRTOS mock CMakeLists file
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
target_sources(${APP_EXECUTABLE_NAME} PRIVATE
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
