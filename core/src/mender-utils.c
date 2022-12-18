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
#include "mender-utils.h"

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
                    /* Unable to allocate memory */
                    regfree(&regex);
                    free(output);
                    mender_log_error("Unable to allocate memory");
                    return NULL;
                }
                output = tmp;

                /* Copy string from previous match to the beginning of the current match */
                memcpy(&output[index], &input[previous_match_finish], current_match_start - previous_match_finish);
                index += (current_match_start - previous_match_finish);
                output[index] = 0;

                /* Reallocate output memory */
                if (NULL == (tmp = (char *)realloc(output, index + strlen(replace) + 1))) {
                    /* Unable to allocate memory */
                    regfree(&regex);
                    free(output);
                    mender_log_error(NULL, "Unable to allocate memory");
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
        /* Unable to allocate memory */
        regfree(&regex);
        free(output);
        mender_log_error(NULL, "Unable to allocate memory");
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
