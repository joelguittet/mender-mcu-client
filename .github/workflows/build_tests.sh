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

# Build weak use case
cmake .. -G "Unix Makefiles" -DCONFIG_MENDER_PLATFORM_FLASH_TYPE="generic/weak" -DCONFIG_MENDER_PLATFORM_LOG_TYPE="generic/weak" -DCONFIG_MENDER_PLATFORM_NET_TYPE="generic/weak" -DCONFIG_MENDER_PLATFORM_RTOS_TYPE="generic/weak" -DCONFIG_MENDER_PLATFORM_STORAGE_TYPE="generic/weak" -DCONFIG_MENDER_PLATFORM_TLS_TYPE="generic/weak" -DCONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE=ON -DCONFIG_MENDER_CLIENT_CONFIGURE_STORAGE=ON -DCONFIG_MENDER_CLIENT_ADD_ON_INVENTORY=ON -DCONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT=ON
make -j$(nproc)

# Build ESP-IDF use case
cmake .. -G "Unix Makefiles" -DCONFIG_MENDER_PLATFORM_FLASH_TYPE="esp-idf" -DCONFIG_MENDER_PLATFORM_LOG_TYPE="esp-idf" -DCONFIG_MENDER_PLATFORM_NET_TYPE="esp-idf" -DCONFIG_MENDER_PLATFORM_RTOS_TYPE="freertos" -DCONFIG_MENDER_PLATFORM_STORAGE_TYPE="esp-idf/nvs" -DCONFIG_MENDER_PLATFORM_TLS_TYPE="generic/mbedtls" -DCONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE=ON -DCONFIG_MENDER_CLIENT_CONFIGURE_STORAGE=ON -DCONFIG_MENDER_CLIENT_ADD_ON_INVENTORY=ON -DCONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT=ON
make -j$(nproc)
cmake .. -G "Unix Makefiles" -DCONFIG_MENDER_PLATFORM_FLASH_TYPE="esp-idf" -DCONFIG_MENDER_PLATFORM_LOG_TYPE="esp-idf" -DCONFIG_MENDER_PLATFORM_NET_TYPE="esp-idf" -DCONFIG_MENDER_PLATFORM_RTOS_TYPE="freertos" -DCONFIG_MENDER_PLATFORM_STORAGE_TYPE="esp-idf/nvs" -DCONFIG_MENDER_PLATFORM_TLS_TYPE="generic/mbedtls" -DCONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE=ON -DCONFIG_MENDER_CLIENT_CONFIGURE_STORAGE=OFF -DCONFIG_MENDER_CLIENT_ADD_ON_INVENTORY=ON -DCONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT=ON
make -j$(nproc)
cmake .. -G "Unix Makefiles" -DCONFIG_MENDER_PLATFORM_FLASH_TYPE="esp-idf" -DCONFIG_MENDER_PLATFORM_LOG_TYPE="esp-idf" -DCONFIG_MENDER_PLATFORM_NET_TYPE="esp-idf" -DCONFIG_MENDER_PLATFORM_RTOS_TYPE="freertos" -DCONFIG_MENDER_PLATFORM_STORAGE_TYPE="esp-idf/nvs" -DCONFIG_MENDER_PLATFORM_TLS_TYPE="generic/cryptoauthlib" -DCONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE=ON -DCONFIG_MENDER_CLIENT_CONFIGURE_STORAGE=ON -DCONFIG_MENDER_CLIENT_ADD_ON_INVENTORY=ON -DCONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT=ON
make -j$(nproc)

# Build Zephyr use case
cmake .. -G "Unix Makefiles" -DCONFIG_MENDER_PLATFORM_FLASH_TYPE="zephyr" -DCONFIG_MENDER_PLATFORM_LOG_TYPE="zephyr" -DCONFIG_MENDER_PLATFORM_NET_TYPE="zephyr" -DCONFIG_MENDER_PLATFORM_RTOS_TYPE="zephyr" -DCONFIG_MENDER_PLATFORM_STORAGE_TYPE="zephyr/nvs" -DCONFIG_MENDER_PLATFORM_TLS_TYPE="generic/mbedtls" -DCONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE=ON -DCONFIG_MENDER_CLIENT_CONFIGURE_STORAGE=ON -DCONFIG_MENDER_CLIENT_ADD_ON_INVENTORY=ON -DCONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT=ON
make -j$(nproc)
cmake .. -G "Unix Makefiles" -DCONFIG_MENDER_PLATFORM_FLASH_TYPE="zephyr" -DCONFIG_MENDER_PLATFORM_LOG_TYPE="zephyr" -DCONFIG_MENDER_PLATFORM_NET_TYPE="zephyr" -DCONFIG_MENDER_PLATFORM_RTOS_TYPE="zephyr" -DCONFIG_MENDER_PLATFORM_STORAGE_TYPE="zephyr/nvs" -DCONFIG_MENDER_PLATFORM_TLS_TYPE="generic/mbedtls" -DCONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE=ON -DCONFIG_MENDER_CLIENT_CONFIGURE_STORAGE=OFF -DCONFIG_MENDER_CLIENT_ADD_ON_INVENTORY=ON -DCONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT=ON
make -j$(nproc)
cmake .. -G "Unix Makefiles" -DCONFIG_MENDER_PLATFORM_FLASH_TYPE="zephyr" -DCONFIG_MENDER_PLATFORM_LOG_TYPE="zephyr" -DCONFIG_MENDER_PLATFORM_NET_TYPE="zephyr" -DCONFIG_MENDER_PLATFORM_RTOS_TYPE="zephyr" -DCONFIG_MENDER_PLATFORM_STORAGE_TYPE="zephyr/nvs" -DCONFIG_MENDER_PLATFORM_TLS_TYPE="generic/cryptoauthlib" -DCONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE=ON -DCONFIG_MENDER_CLIENT_CONFIGURE_STORAGE=ON -DCONFIG_MENDER_CLIENT_ADD_ON_INVENTORY=ON -DCONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT=ON
make -j$(nproc)

# Build Posix use case
cmake .. -G "Unix Makefiles" -DCONFIG_MENDER_PLATFORM_FLASH_TYPE="posix" -DCONFIG_MENDER_PLATFORM_LOG_TYPE="posix" -DCONFIG_MENDER_PLATFORM_NET_TYPE="generic/curl" -DCONFIG_MENDER_PLATFORM_RTOS_TYPE="posix" -DCONFIG_MENDER_PLATFORM_STORAGE_TYPE="posix" -DCONFIG_MENDER_PLATFORM_TLS_TYPE="generic/mbedtls" -DCONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE=ON -DCONFIG_MENDER_CLIENT_CONFIGURE_STORAGE=ON -DCONFIG_MENDER_CLIENT_ADD_ON_INVENTORY=ON -DCONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT=ON
make -j$(nproc)
cmake .. -G "Unix Makefiles" -DCONFIG_MENDER_PLATFORM_FLASH_TYPE="posix" -DCONFIG_MENDER_PLATFORM_LOG_TYPE="posix" -DCONFIG_MENDER_PLATFORM_NET_TYPE="generic/curl" -DCONFIG_MENDER_PLATFORM_RTOS_TYPE="posix" -DCONFIG_MENDER_PLATFORM_STORAGE_TYPE="posix" -DCONFIG_MENDER_PLATFORM_TLS_TYPE="generic/mbedtls" -DCONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE=ON -DCONFIG_MENDER_CLIENT_CONFIGURE_STORAGE=OFF -DCONFIG_MENDER_CLIENT_ADD_ON_INVENTORY=ON -DCONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT=ON
make -j$(nproc)
