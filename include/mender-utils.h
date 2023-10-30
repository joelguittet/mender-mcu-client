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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <cJSON.h>

/**
 * @brief Mender error codes
 */
typedef enum {
    MENDER_DONE            = 1,  /**< Done */
    MENDER_OK              = 0,  /**< OK */
    MENDER_FAIL            = -1, /**< Failure */
    MENDER_NOT_FOUND       = -2, /**< Not found */
    MENDER_NOT_IMPLEMENTED = -3, /**< Not implemented */
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
 * @brief Key-store item
 */
typedef struct {
    char *name;  /**< Name of the item */
    char *value; /**< Value of the item */
} mender_keystore_item_t;

/**
 * @brief Key-store
 */
typedef mender_keystore_item_t mender_keystore_t;

/**
 * @brief Function used to print HTTP status as string
 * @param status HTTP status code
 * @return HTTP status as string, NULL if it is not found
 */
char *mender_utils_http_status_to_string(int status);

/**
 * @brief Function used to print deployment status as string
 * @param deployment_status Deployment status
 * @return Deployment status as string, NULL if it is not found
 */
char *mender_utils_deployment_status_to_string(mender_deployment_status_t deployment_status);

/**
 * @brief Function used to locate last substring in string
 * @param haystack String to look for a substring
 * @param needle Substring to look for
 * @return Pointer to the beginning of the substring, NULL is the substring is not found
 */
char *mender_utils_strrstr(const char *haystack, const char *needle);

/**
 * @brief Function used to check if string begins with wanted substring
 * @param s1 String to be checked
 * @param s2 Substring to look for at the beginning of the string
 * @return true if the string begins with wanted substring, false otherwise
 */
bool mender_utils_strbeginwith(const char *s1, const char *s2);

/**
 * @brief Function used to check if string ends with wanted substring
 * @param s1 String to be checked
 * @param s2 Substring to look for at the end of the string
 * @return true if the string ends with wanted substring, false otherwise
 */
bool mender_utils_strendwith(const char *s1, const char *s2);

/**
 * @brief Function used to replace a string in the input buffer
 * @param input Input buffer
 * @param search String to be replaced or regex expression
 * @param replace Replacement string
 * @return New string with replacements if the function succeeds, NULL otherwise
 */
char *mender_utils_str_replace(char *input, char *search, char *replace);

/**
 * @brief Function used to create a key-store
 * @param length Length of the key-store
 * @return Key-store if the function succeeds, NULL otherwise
 */
mender_keystore_t *mender_utils_keystore_new(size_t length);

/**
 * @brief Function used to copy key-store
 * @param dst_keystore Destination key-store to create
 * @param src_keystore Source key-store to copy
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_utils_keystore_copy(mender_keystore_t **dst_keystore, mender_keystore_t *src_keystore);

/**
 * @brief Function used to set key-store from JSON string
 * @param keystore Key-store
 * @param data JSON string
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_utils_keystore_from_string(mender_keystore_t **keystore, char *data);

/**
 * @brief Function used to format key-store to JSON string
 * @param keystore Key-store
 * @param data JSON string
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_utils_keystore_to_string(mender_keystore_t *keystore, char **data);

/**
 * @brief Function used to set key-store item name and value
 * @param keystore Key-store to be updated
 * @param index Index of the item in the key-store
 * @param name Name of the item
 * @param value Value of the item
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_utils_keystore_set_item(mender_keystore_t *keystore, size_t index, char *name, char *value);

/**
 * @brief Function used to get length of key-store
 * @param keystore Key-store
 * @return Length of the key-store
 */
size_t mender_utils_keystore_length(mender_keystore_t *keystore);

/**
 * @brief Function used to delete key-store
 * @param keystore Key-store to be deleted
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_utils_keystore_delete(mender_keystore_t *keystore);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_UTILS_H__ */
