/**
 * @file      mender-utils.h
 * @brief     Mender utility functions
 *
 * MIT License
 *
 * Copyright (c) 2022-2023 joelguittet and mender-mcu-client contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __MENDER_UTILS_H__
#define __MENDER_UTILS_H__

#include "mender-common.h"

/**
 * @brief Function used to print HTTP status as string
 * @param status HTTP status code
 * @return HTTP status as string, NULL if it is not found
 */
char *mender_utils_http_status_to_string(int status);

/**
 * @brief Function used to print deployment status as string
 * @param deployment_status Deployment status
 * @return HTTP status as string, NULL if it is not found
 */
char *mender_utils_deployment_status_to_string(mender_deployment_status_t deployment_status);

#endif /* __MENDER_UTILS_H__ */
