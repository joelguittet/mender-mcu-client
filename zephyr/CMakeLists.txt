# @file      CMakeLists.txt
# @brief     mender-mcu-client module CMakeLists file
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

if(CONFIG_MENDER_MCU_CLIENT)
    zephyr_include_directories(../include)
    zephyr_library()
    zephyr_library_sources(
        ../core/src/mender-api.c
        ../core/src/mender-client.c
        ../core/src/mender-untar.c
        ../core/src/mender-utils.c
        ../platform/board/zephyr/src/mender-log.c
        ../platform/board/zephyr/src/mender-ota.c
        ../platform/board/zephyr/src/mender-storage.c
        ../platform/http/zephyr/src/mender-http.c
        ../platform/rtos/zephyr/src/mender-rtos.c
        ../platform/tls/mbedtls/src/mender-tls.c
    )
endif()
