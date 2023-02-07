#!/bin/bash
# @file      build_tests.sh
# @brief     Build all tests
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

cd tests
mkdir build
cd build

# Build ESP-IDF use case
cmake .. -G "Unix Makefiles" -DCONFIG_MENDER_MCU_CLIENT_BOARD_TYPE="esp-idf" -DCONFIG_MENDER_MCU_CLIENT_HTTP_TYPE="esp-idf" -DCONFIG_MENDER_MCU_CLIENT_RTOS_TYPE="freertos" -DCONFIG_MENDER_MCU_CLIENT_TLS_TYPE="mbedtls"
make -j$(nproc)

# Build Zephyr use case
cmake .. -G "Unix Makefiles" -DCONFIG_MENDER_MCU_CLIENT_BOARD_TYPE="zephyr" -DCONFIG_MENDER_MCU_CLIENT_HTTP_TYPE="zephyr" -DCONFIG_MENDER_MCU_CLIENT_RTOS_TYPE="zephyr" -DCONFIG_MENDER_MCU_CLIENT_TLS_TYPE="mbedtls"
make -j$(nproc)