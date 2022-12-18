/**
 * @file      mender-common.h
 * @brief     Mender common definitions
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

#ifndef __MENDER_COMMON_H__
#define __MENDER_COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

/**
 * @brief Mender error codes
 */
typedef enum {
    MENDER_OK        = 0,  /**< OK */
    MENDER_FAIL      = -1, /**< Failure */
    MENDER_NOT_FOUND = -2, /**< Not found */
} mender_err_t;

/**
 * @brief Deployment status
 */
typedef enum {
    MENDER_DEPLOYMENT_STATUS_DOWNLOADING,      /**< Status is "downloading" */
    MENDER_DEPLOYMENT_STATUS_INSTALLING,       /**< Status is "installing" */
    MENDER_DEPLOYMENT_STATUS_REBOOTING,        /**< Status is "rebooting" */
    MENDER_DEPLOYMENT_STATUS_SUCCESS,          /**< Status is "success" */
    MENDER_DEPLOYMENT_STATUS_FAILURE,          /**< Status is "failure" */
    MENDER_DEPLOYMENT_STATUS_ALREADY_INSTALLED /**< Status is "already installed" */
} mender_deployment_status_t;

/**
 * @brief Inventory item
 */
typedef struct {
    char *name;  /**< Name of the item */
    char *value; /**< Value of the item */
} mender_inventory_t;

#endif /* __MENDER_COMMON_H__ */
