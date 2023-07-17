# @file      CMakeLists.txt
# @brief     Mocks CMakeLists file
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

# Include all mocks
include("${CMAKE_CURRENT_LIST_DIR}/cjson/CMakeLists.txt")
if(CONFIG_MENDER_MCU_CLIENT_BOARD_TYPE MATCHES "esp-idf")
    include("${CMAKE_CURRENT_LIST_DIR}/esp-idf/CMakeLists.txt")
endif()
if(CONFIG_MENDER_MCU_CLIENT_RTOS_TYPE MATCHES "freertos")
    include("${CMAKE_CURRENT_LIST_DIR}/freertos/CMakeLists.txt")
endif()
if(CONFIG_MENDER_MCU_CLIENT_RTOS_TYPE MATCHES "zephyr")
    include("${CMAKE_CURRENT_LIST_DIR}/zephyr/CMakeLists.txt")
endif()
if(CONFIG_MENDER_MCU_CLIENT_TLS_TYPE MATCHES "mbedtls")
    include("${CMAKE_CURRENT_LIST_DIR}/mbedtls/CMakeLists.txt")
endif()
