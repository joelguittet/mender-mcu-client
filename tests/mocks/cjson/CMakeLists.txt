# @file      CMakeLists.txt
# @brief     cJSON mock CMakeLists file
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
set(GIT_REPO_URL "https://github.com/DaveGamble/cJSON")
set(GIT_TAG_NAME "v1.7.15")

# Fetch destination
set(GIT_FOLDER_NAME "${CMAKE_CURRENT_LIST_DIR}/cjson")

# Declare fetch content
include(FetchContent)
FetchContent_Declare(
    cjson
    URL             "${GIT_REPO_URL}/archive/refs/tags/${GIT_TAG_NAME}.zip"
    SOURCE_DIR      "${GIT_FOLDER_NAME}"
    SOURCE_SUBDIR   "none" # Allow to not include automatically the repository CMakeLists.txt file
)

# Fetch if not already done
message("Populating ${GIT_FOLDER_NAME}, please wait...")
FetchContent_MakeAvailable(cjson)

# Add sources
target_sources(${EXECUTABLE_NAME} PRIVATE "${GIT_FOLDER_NAME}/cJSON.c")

# Add include directories
include_directories(${GIT_FOLDER_NAME})
