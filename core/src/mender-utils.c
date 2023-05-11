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

#include "mender-log.h"
#include "mender-utils.h"

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

    char *r = NULL;

    if (!needle[0])
        return (char *)haystack + strlen(haystack);

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
