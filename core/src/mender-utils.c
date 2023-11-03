/**
 * @file      mender-utils.c
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

#include <sys/types.h>
#include <regex.h>
#include "mender-log.h"

char *
mender_utils_http_status_to_string(int status) {

    /* Definition of status strings */
    const struct {
        uint16_t status;
        char *   str;
    } desc[] = { { 100, "Continue" },
                 { 101, "Switching Protocols" },
                 { 103, "Early Hints" },
                 { 200, "OK" },
                 { 201, "Created" },
                 { 202, "Accepted" },
                 { 203, "Non-Authoritative Information" },
                 { 204, "No Content" },
                 { 205, "Reset Content" },
                 { 206, "Partial Content" },
                 { 300, "Multiple Choices" },
                 { 301, "Moved Permanently" },
                 { 302, "Found" },
                 { 303, "See Other" },
                 { 304, "Not Modified" },
                 { 307, "Temporary Redirect" },
                 { 308, "Permanent Redirect" },
                 { 400, "Bad Request" },
                 { 401, "Unauthorized" },
                 { 402, "Payment Required" },
                 { 403, "Forbidden" },
                 { 404, "Not Found" },
                 { 405, "Method Not Allowed" },
                 { 406, "Not Acceptable" },
                 { 407, "Proxy Authentication Required" },
                 { 408, "Request Timeout" },
                 { 409, "Conflict" },
                 { 410, "Gone" },
                 { 411, "Length Required" },
                 { 412, "Precondition Failed" },
                 { 413, "Payload Too Large" },
                 { 414, "URI Too Long" },
                 { 415, "Unsupported Media Type" },
                 { 416, "Range Not Satisfiable" },
                 { 417, "Expectation Failed" },
                 { 418, "I'm a teapot" },
                 { 422, "Unprocessable Entity" },
                 { 425, "Too Early" },
                 { 426, "Upgrade Required" },
                 { 428, "Precondition Required" },
                 { 429, "Too Many Requests" },
                 { 431, "Request Header Fields Too Large" },
                 { 451, "Unavailable For Legal Reasons" },
                 { 500, "Internal Server Error" },
                 { 501, "Not Implemented" },
                 { 502, "Bad Gateway" },
                 { 503, "Service Unavailable" },
                 { 504, "Gateway Timeout" },
                 { 505, "HTTP Version Not Supported" },
                 { 506, "Variant Also Negotiates" },
                 { 507, "Insufficient Storage" },
                 { 508, "Loop Detected" },
                 { 510, "Not Extended" },
                 { 511, "Network Authentication Required" } };

    /* Return HTTP status as string */
    for (int index = 0; index < sizeof(desc) / sizeof(desc[0]); index++) {
        if (desc[index].status == status) {
            return desc[index].str;
        }
    }

    return NULL;
}

char *
mender_utils_strrstr(const char *haystack, const char *needle) {

    assert(NULL != haystack);
    assert(NULL != needle);

    char *r = NULL;

    if (!needle[0]) {
        return (char *)haystack + strlen(haystack);
    }

    while (1) {
        char *p = strstr(haystack, needle);
        if (!p) {
            return r;
        }
        r        = p;
        haystack = p + 1;
    }
}

bool
mender_utils_strbeginwith(const char *s1, const char *s2) {

    /* Check parameters */
    if ((NULL == s1) || (NULL == s2)) {
        return false;
    }

    /* Compare the begining of the string */
    return (0 == strncmp(s1, s2, strlen(s2)));
}

bool
mender_utils_strendwith(const char *s1, const char *s2) {

    /* Check parameters */
    if ((NULL == s1) || (NULL == s2)) {
        return false;
    }

    /* Compare the end of the string */
    return (0 == strncmp(s1 + strlen(s1) - strlen(s2), s2, strlen(s2)));
}

char *
mender_utils_str_replace(char *input, char *search, char *replace) {

    assert(NULL != input);
    assert(NULL != search);
    assert(NULL != replace);

    regex_t    regex;
    regmatch_t match;
    char *     str                   = input;
    char *     output                = NULL;
    size_t     index                 = 0;
    int        previous_match_finish = 0;

    /* Compile expression */
    if (0 != regcomp(&regex, search, REG_EXTENDED)) {
        /* Unable to compile expression */
        mender_log_error("Unable to compile expression '%s'", search);
        return NULL;
    }

    /* Loop until all search string are replaced */
    bool loop = true;
    while (true == loop) {

        /* Search wanted string */
        if (0 != regexec(&regex, str, 1, &match, 0)) {
            /* No more string to be replaced */
            loop = false;
        } else {
            if (match.rm_so != -1) {

                /* Beginning and ending offset of the match */
                int current_match_start  = (int)(match.rm_so + (str - input));
                int current_match_finish = (int)(match.rm_eo + (str - input));

                /* Reallocate output memory */
                char *tmp = (char *)realloc(output, index + (current_match_start - previous_match_finish) + 1);
                if (NULL == tmp) {
                    mender_log_error("Unable to allocate memory");
                    regfree(&regex);
                    free(output);
                    return NULL;
                }
                output = tmp;

                /* Copy string from previous match to the beginning of the current match */
                memcpy(&output[index], &input[previous_match_finish], current_match_start - previous_match_finish);
                index += (current_match_start - previous_match_finish);
                output[index] = 0;

                /* Reallocate output memory */
                if (NULL == (tmp = (char *)realloc(output, index + strlen(replace) + 1))) {
                    mender_log_error(NULL, "Unable to allocate memory");
                    regfree(&regex);
                    free(output);
                    return NULL;
                }
                output = tmp;

                /* Copy replace string to the output */
                strcat(output, replace);
                index += strlen(replace);

                /* Update previous match ending value */
                previous_match_finish = current_match_finish;
            }
            str += match.rm_eo;
        }
    }

    /* Reallocate output memory */
    char *tmp = (char *)realloc(output, index + (strlen(input) - previous_match_finish) + 1);
    if (NULL == tmp) {
        mender_log_error(NULL, "Unable to allocate memory");
        regfree(&regex);
        free(output);
        return NULL;
    }
    output = tmp;

    /* Copy the end of the string after the latest match */
    memcpy(&output[index], &input[previous_match_finish], strlen(input) - previous_match_finish);
    index += (strlen(input) - previous_match_finish);
    output[index] = 0;

    /* Release regex */
    regfree(&regex);

    return output;
}

char *
mender_utils_deployment_status_to_string(mender_deployment_status_t deployment_status) {

    /* Return deployment status as string */
    if (MENDER_DEPLOYMENT_STATUS_DOWNLOADING == deployment_status) {
        return "downloading";
    } else if (MENDER_DEPLOYMENT_STATUS_INSTALLING == deployment_status) {
        return "installing";
    } else if (MENDER_DEPLOYMENT_STATUS_REBOOTING == deployment_status) {
        return "rebooting";
    } else if (MENDER_DEPLOYMENT_STATUS_SUCCESS == deployment_status) {
        return "success";
    } else if (MENDER_DEPLOYMENT_STATUS_FAILURE == deployment_status) {
        return "failure";
    } else if (MENDER_DEPLOYMENT_STATUS_ALREADY_INSTALLED == deployment_status) {
        return "already-installed";
    }

    return NULL;
}

mender_keystore_t *
mender_utils_keystore_new(size_t length) {

    /* Allocate memory */
    mender_keystore_t *keystore = (mender_keystore_t *)malloc((length + 1) * sizeof(mender_keystore_item_t));
    if (NULL == keystore) {
        mender_log_error("Unable to allocate memory");
        return NULL;
    }

    /* Initialize keystore */
    memset(keystore, 0, (length + 1) * sizeof(mender_keystore_item_t));

    return keystore;
}

mender_err_t
mender_utils_keystore_copy(mender_keystore_t **dst_keystore, mender_keystore_t *src_keystore) {

    assert(NULL != dst_keystore);
    mender_err_t ret = MENDER_OK;

    /* Copy the new keystore */
    size_t length = mender_utils_keystore_length(src_keystore);
    if (NULL == (*dst_keystore = mender_utils_keystore_new(length))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    for (size_t index = 0; index < length; index++) {
        if (MENDER_OK != (ret = mender_utils_keystore_set_item(*dst_keystore, index, src_keystore[index].name, src_keystore[index].value))) {
            mender_log_error("Unable to allocate memory");
            goto END;
        }
    }

END:

    return ret;
}

mender_err_t
mender_utils_keystore_from_string(mender_keystore_t **keystore, char *data) {

    assert(NULL != keystore);
    mender_err_t ret;

    /* Release previous keystore */
    if (MENDER_OK != (ret = mender_utils_keystore_delete(*keystore))) {
        mender_log_error("Unable to delete keystore");
        return ret;
    }
    *keystore = NULL;

    /* Set key-store */
    if (NULL != data) {
        cJSON *json_data = cJSON_Parse(data);
        if (NULL != json_data) {
            size_t length       = 0;
            cJSON *current_item = json_data->child;
            while (NULL != current_item) {
                if ((NULL != current_item->string) && (NULL != current_item->valuestring)) {
                    length++;
                }
                current_item = current_item->next;
            }
            if (NULL != (*keystore = mender_utils_keystore_new(length))) {
                size_t index = 0;
                current_item = json_data->child;
                while (NULL != current_item) {
                    if ((NULL != current_item->string) && (NULL != current_item->valuestring)) {
                        if (MENDER_OK != (ret = mender_utils_keystore_set_item(*keystore, index, current_item->string, current_item->valuestring))) {
                            mender_log_error("Unable to allocate memory");
                            cJSON_Delete(json_data);
                            return ret;
                        }
                        index++;
                    }
                    current_item = current_item->next;
                }
            } else {
                mender_log_error("Unable to allocate memory");
                ret = MENDER_FAIL;
            }
            cJSON_Delete(json_data);
        } else {
            mender_log_error("Invalid data");
            ret = MENDER_FAIL;
        }
    }

    return ret;
}

mender_err_t
mender_utils_keystore_to_string(mender_keystore_t *keystore, char **data) {

    assert(NULL != data);
    mender_err_t ret = MENDER_OK;

    /* Format data */
    cJSON *object = cJSON_CreateObject();
    if (NULL == object) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    if (NULL != keystore) {
        size_t index = 0;
        while ((NULL != keystore[index].name) && (NULL != keystore[index].value)) {
            cJSON_AddStringToObject(object, keystore[index].name, keystore[index].value);
            index++;
        }
    }
    if (NULL == (*data = cJSON_PrintUnformatted(object))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }

END:

    /* Release memory */
    if (NULL != object) {
        cJSON_Delete(object);
    }

    return ret;
}

mender_err_t
mender_utils_keystore_set_item(mender_keystore_t *keystore, size_t index, char *name, char *value) {

    assert(NULL != keystore);

    /* Release memory */
    if (NULL != keystore[index].name) {
        free(keystore[index].name);
        keystore[index].name = NULL;
    }
    if (NULL != keystore[index].value) {
        free(keystore[index].value);
        keystore[index].value = NULL;
    }

    /* Copy name and value */
    if (NULL != name) {
        if (NULL == (keystore[index].name = strdup(name))) {
            mender_log_error("Unable to allocate memory");
            return MENDER_FAIL;
        }
    }
    if (NULL != value) {
        if (NULL == (keystore[index].value = strdup(value))) {
            mender_log_error("Unable to allocate memory");
            return MENDER_FAIL;
        }
    }

    return MENDER_OK;
}

size_t
mender_utils_keystore_length(mender_keystore_t *keystore) {

    /* Compute key-store length */
    size_t length = 0;
    if (NULL != keystore) {
        while ((NULL != keystore[length].name) && (NULL != keystore[length].value)) {
            length++;
        }
    }

    return length;
}

mender_err_t
mender_utils_keystore_delete(mender_keystore_t *keystore) {

    /* Release memory */
    if (NULL != keystore) {
        size_t index = 0;
        while ((NULL != keystore[index].name) || (NULL != keystore[index].value)) {
            if (NULL != keystore[index].name) {
                free(keystore[index].name);
            }
            if (NULL != keystore[index].value) {
                free(keystore[index].value);
            }
            index++;
        }
        free(keystore);
    }

    return MENDER_OK;
}
