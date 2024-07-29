# @file      CMakeLists.txt
# @brief     cryptoauthlib mock CMakeLists file
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
set(GIT_REPO_URL "https://github.com/MicrochipTech/cryptoauthlib")
set(GIT_TAG_NAME "v3.5.1")

# Fetch destination
set(GIT_FOLDER_NAME "${CMAKE_CURRENT_LIST_DIR}/cryptoauthlib")

# Declare fetch content
include(FetchContent)
FetchContent_Declare(
    cryptoauthlib
    URL                        "${GIT_REPO_URL}/archive/refs/tags/${GIT_TAG_NAME}.zip"
    SOURCE_DIR                 "${GIT_FOLDER_NAME}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# Fetch if not already done
FetchContent_GetProperties(cryptoauthlib)
if(NOT cryptoauthlib_POPULATED)
    message("Populating ${GIT_FOLDER_NAME}, please wait...")
    FetchContent_Populate(cryptoauthlib)
endif()

# Configure cryptoauthlib
SET(ATCA_HAL_I2C ON CACHE INTERNAL "")
SET(ATCA_ATSHA204A_SUPPORT OFF CACHE INTERNAL "")
SET(ATCA_ATSHA206A_SUPPORT OFF CACHE INTERNAL "")
SET(ATCA_ATECC108A_SUPPORT OFF CACHE INTERNAL "")
SET(ATCA_ATECC508A_SUPPORT OFF CACHE INTERNAL "")
SET(ATCA_ATECC608_SUPPORT ON CACHE INTERNAL "")
SET(ATCA_ATECC608A_SUPPORT ON CACHE INTERNAL "")
SET(ATCA_TA100_SUPPORT OFF CACHE INTERNAL "")
SET(ATCA_ECC204_SUPPORT OFF CACHE INTERNAL "")
SET(ATCA_TA010_SUPPORT OFF CACHE INTERNAL "")
SET(ATCA_SHA104_SUPPORT OFF CACHE INTERNAL "")
SET(ATCA_SHA105_SUPPORT OFF CACHE INTERNAL "")

# Include library
add_subdirectory("${GIT_FOLDER_NAME}/lib")

# Add include directories
include_directories("${CMAKE_CURRENT_BINARY_DIR}/mocks/cryptoauthlib/cryptoauthlib/lib")
include_directories("${GIT_FOLDER_NAME}/lib")
