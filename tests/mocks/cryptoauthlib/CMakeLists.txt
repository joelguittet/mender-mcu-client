# @file      CMakeLists.txt
# @brief     cryptoauthlib mock CMakeLists file
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
