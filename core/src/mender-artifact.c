/**
 * @file      mender-artifact.c
 * @brief     Mender artifact parser
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

#include "mender-artifact.h"
#include "mender-log.h"

/**
 * @brief TAR block size
 */
#define MENDER_ARTIFACT_STREAM_BLOCK_SIZE (512)

/**
 * @brief TAR file header
 */
typedef struct {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
} mender_artifact_tar_header_t;

/**
 * @brief Mender artifact format version
 */
#define MENDER_ARTIFACT_VERSION_FORMAT "mender"
#define MENDER_ARTIFACT_VERSION_VALUE  3

/**
 * @brief Parse header of TAR file
 * @param ctx Artifact context
 * @return MENDER_DONE if the data have been parsed, MENDER_OK if there is not enough data to parse, error code if an error occurred
 */
static mender_err_t mender_artifact_parse_tar_header(mender_artifact_ctx_t *ctx);

/**
 * @brief Check version file of the artifact
 * @param ctx Artifact context
 * @return MENDER_DONE if the data have been parsed and version verified, MENDER_OK if there is not enough data to parse, error code if an error occurred
 */
static mender_err_t mender_artifact_check_version(mender_artifact_ctx_t *ctx);

/**
 * @brief Read header-info file of the artifact
 * @param ctx Artifact context
 * @return MENDER_DONE if the data have been parsed and payloads retrieved, MENDER_OK if there is not enough data to parse, error code if an error occurred
 */
static mender_err_t mender_artifact_read_header_info(mender_artifact_ctx_t *ctx);

/**
 * @brief Read meta-data file of the artifact
 * @param ctx Artifact context
 * @return MENDER_DONE if the data have been parsed and payloads retrieved, MENDER_OK if there is not enough data to parse, error code if an error occurred
 */
static mender_err_t mender_artifact_read_meta_data(mender_artifact_ctx_t *ctx);

/**
 * @brief Read data file of the artifact
 * @param ctx Artifact context
 * @param callback Callback function to be invoked to perform the treatment of the data from the artifact
 * @return MENDER_DONE if the data have been parsed and payloads retrieved, MENDER_OK if there is not enough data to parse, error code if an error occurred
 */
static mender_err_t mender_artifact_read_data(mender_artifact_ctx_t *ctx, mender_err_t (*callback)(char *, cJSON *, char *, size_t, void *, size_t, size_t));

/**
 * @brief Drop content of the current file of the artifact
 * @param ctx Artifact context
 * @return MENDER_DONE if the data have been parsed and dropped, MENDER_OK if there is not enough data to parse, error code if an error occurred
 */
static mender_err_t mender_artifact_drop_file(mender_artifact_ctx_t *ctx);

/**
 * @brief Shift data after parsing
 * @param ctx Artifact context
 * @param length Length of data to shift
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_artifact_shift_data(mender_artifact_ctx_t *ctx, size_t length);

/**
 * @brief Compute length rounded up to increment (usually the block size)
 * @param length Length
 * @param incr Increment
 * @return Rounded length
 */
static size_t mender_artifact_round_up(size_t length, size_t incr);

mender_artifact_ctx_t *
mender_artifact_create_ctx(void) {

    mender_artifact_ctx_t *ctx;

    /* Create new context */
    if (NULL == (ctx = (mender_artifact_ctx_t *)malloc(sizeof(mender_artifact_ctx_t)))) {
        return NULL;
    }
    memset(ctx, 0, sizeof(mender_artifact_ctx_t));

    return ctx;
}

mender_err_t
mender_artifact_process_data(mender_artifact_ctx_t *ctx,
                             void *                 input_data,
                             size_t                 input_length,
                             mender_err_t (*callback)(char *, cJSON *, char *, size_t, void *, size_t, size_t)) {

    assert(NULL != ctx);
    assert(NULL != callback);
    mender_err_t ret = MENDER_OK;
    void *       tmp;

    /* Copy data to the end of the internal buffer */
    if ((NULL != input_data) && (0 != input_length)) {
        if (NULL == (tmp = realloc(ctx->input.data, ctx->input.length + input_length))) {
            /* Unable to allocate memory */
            return MENDER_FAIL;
        }
        ctx->input.data = tmp;
        memcpy((void *)(((uint8_t *)ctx->input.data) + ctx->input.length), input_data, input_length);
        ctx->input.length += input_length;
    }

    /* Parse data */
    do {

        /* Treatment depending of the stream state */
        if (MENDER_ARTIFACT_STREAM_STATE_PARSING_HEADER == ctx->stream_state) {

            /* Parse TAR header */
            ret = mender_artifact_parse_tar_header(ctx);

        } else if (MENDER_ARTIFACT_STREAM_STATE_PARSING_DATA == ctx->stream_state) {

            /* Treatment depending of the file name */
            if (!strcmp(ctx->file.name, "version")) {

                /* Validate artifact version */
                ret = mender_artifact_check_version(ctx);

            } else if (!strcmp(ctx->file.name, "header.tar/header-info")) {

                /* Read header-info file */
                ret = mender_artifact_read_header_info(ctx);

            } else if ((true == mender_utils_strbeginwith(ctx->file.name, "header.tar/headers"))
                       && (true == mender_utils_strendwith(ctx->file.name, "meta-data"))) {

                /* Read meta-data file */
                ret = mender_artifact_read_meta_data(ctx);

            } else if (true == mender_utils_strbeginwith(ctx->file.name, "data")) {

                /* Read data */
                ret = mender_artifact_read_data(ctx, callback);

            } else if (false == mender_utils_strendwith(ctx->file.name, ".tar")) {

                /* Drop data, file is not relevant */
                ret = mender_artifact_drop_file(ctx);

            } else {

                /* Nothing to do */
                ret = MENDER_DONE;
            }

            /* Check if file have been parsed and treatment done */
            if (MENDER_DONE == ret) {

                /* Remove the previous file name */
                char *substring = mender_utils_strrstr(ctx->file.name, ".tar");
                if (NULL != substring) {
                    *(substring + strlen(".tar")) = '\0';
                } else {
                    free(ctx->file.name);
                    ctx->file.name = NULL;
                }
                ctx->file.size  = 0;
                ctx->file.index = 0;

                /* Update the stream state machine */
                ctx->stream_state = MENDER_ARTIFACT_STREAM_STATE_PARSING_HEADER;
            }
        }
    } while (MENDER_DONE == ret);

    return ret;
}

void
mender_artifact_release_ctx(mender_artifact_ctx_t *ctx) {

    /* Release memory */
    if (NULL != ctx) {
        if (NULL != ctx->input.data) {
            free(ctx->input.data);
        }
        if (NULL != ctx->payloads.values) {
            for (size_t index = 0; index < ctx->payloads.size; index++) {
                if (NULL != ctx->payloads.values[index].type) {
                    free(ctx->payloads.values[index].type);
                }
                if (NULL != ctx->payloads.values[index].meta_data) {
                    cJSON_Delete(ctx->payloads.values[index].meta_data);
                }
            }
            free(ctx->payloads.values);
        }
        if (NULL != ctx->file.name) {
            free(ctx->file.name);
        }
        free(ctx);
    }
}

static mender_err_t
mender_artifact_parse_tar_header(mender_artifact_ctx_t *ctx) {

    assert(NULL != ctx);
    char *tmp;

    /* Check if enough data are received (at least one block) */
    if ((NULL == ctx->input.data) || (ctx->input.length < MENDER_ARTIFACT_STREAM_BLOCK_SIZE)) {
        return MENDER_OK;
    }

    /* Cast block to TAR header structure */
    mender_artifact_tar_header_t *tar_header = (mender_artifact_tar_header_t *)ctx->input.data;

    /* Check if file name is provided, else the end of the current TAR file is reached */
    if ('\0' == tar_header->name[0]) {

        /* Check if enough data are received (at least 2 blocks) */
        if (ctx->input.length < 2 * MENDER_ARTIFACT_STREAM_BLOCK_SIZE) {
            return MENDER_OK;
        }

        /* Remove the TAR file name */
        if (NULL != ctx->file.name) {
            char *substring = mender_utils_strrstr(ctx->file.name, ".tar");
            if (NULL != substring) {
                *substring = '\0';
                substring  = mender_utils_strrstr(ctx->file.name, ".tar");
                if (NULL != substring) {
                    *(substring + strlen(".tar")) = '\0';
                } else {
                    free(ctx->file.name);
                    ctx->file.name = NULL;
                }
            } else {
                free(ctx->file.name);
                ctx->file.name = NULL;
            }
        }

        /* Shift data in the buffer */
        if (MENDER_OK != mender_artifact_shift_data(ctx, 2 * MENDER_ARTIFACT_STREAM_BLOCK_SIZE)) {
            mender_log_error("Unable to shift input data");
            return MENDER_FAIL;
        }

        return MENDER_DONE;
    }

    /* Check magic */
    if (strncmp(tar_header->magic, "ustar", strlen("ustar"))) {
        /* Invalid magic */
        mender_log_error("Invalid magic");
        return MENDER_FAIL;
    }

    /* Compute the new file name */
    if (NULL != ctx->file.name) {
        size_t str_length = strlen(ctx->file.name) + strlen("/") + strlen(tar_header->name) + 1;
        if (NULL == (tmp = (char *)malloc(str_length))) {
            mender_log_error("Unable to allocate memory");
            return MENDER_FAIL;
        }
        snprintf(tmp, str_length, "%s/%s", ctx->file.name, tar_header->name);
        free(ctx->file.name);
    } else {
        if (NULL == (tmp = strdup(tar_header->name))) {
            mender_log_error("Unable to allocate memory");
            return MENDER_FAIL;
        }
    }
    ctx->file.name = tmp;

    /* Retrieve file size */
    sscanf(tar_header->size, "%o", (unsigned int *)&(ctx->file.size));
    ctx->file.index = 0;

    /* Shift data in the buffer */
    if (MENDER_OK != mender_artifact_shift_data(ctx, MENDER_ARTIFACT_STREAM_BLOCK_SIZE)) {
        mender_log_error("Unable to shift input data");
        return MENDER_FAIL;
    }

    /* Update the stream state machine */
    ctx->stream_state = MENDER_ARTIFACT_STREAM_STATE_PARSING_DATA;

    return MENDER_DONE;
}

static mender_err_t
mender_artifact_check_version(mender_artifact_ctx_t *ctx) {

    assert(NULL != ctx);
    cJSON *      object = NULL;
    mender_err_t ret    = MENDER_DONE;

    /* Check if all data have been received */
    if ((NULL == ctx->input.data) || (ctx->input.length < mender_artifact_round_up(ctx->file.size, MENDER_ARTIFACT_STREAM_BLOCK_SIZE))) {
        return MENDER_OK;
    }

    /* Check version file */
    if (NULL == (object = cJSON_ParseWithLength(ctx->input.data, ctx->file.size))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }
    cJSON *json_format = cJSON_GetObjectItemCaseSensitive(object, "format");
    if (true == cJSON_IsString(json_format)) {
        if (strcmp(cJSON_GetStringValue(json_format), MENDER_ARTIFACT_VERSION_FORMAT)) {
            mender_log_error("Invalid version format");
            ret = MENDER_FAIL;
            goto END;
        }
    } else {
        mender_log_error("Invalid version file");
        ret = MENDER_FAIL;
        goto END;
    }
    cJSON *json_version = cJSON_GetObjectItemCaseSensitive(object, "version");
    if (true == cJSON_IsNumber(json_version)) {
        if (MENDER_ARTIFACT_VERSION_VALUE != (int)cJSON_GetNumberValue(json_version)) {
            mender_log_error("Invalid version value");
            ret = MENDER_FAIL;
            goto END;
        }
    } else {
        mender_log_error("Invalid version file");
        ret = MENDER_FAIL;
        goto END;
    }
    mender_log_info("Artifact has valid version");

    /* Shift data in the buffer */
    if (MENDER_OK != mender_artifact_shift_data(ctx, mender_artifact_round_up(ctx->file.size, MENDER_ARTIFACT_STREAM_BLOCK_SIZE))) {
        mender_log_error("Unable to shift input data");
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

static mender_err_t
mender_artifact_read_header_info(mender_artifact_ctx_t *ctx) {

    assert(NULL != ctx);
    cJSON *      object = NULL;
    mender_err_t ret    = MENDER_DONE;

    /* Check if all data have been received */
    if ((NULL == ctx->input.data) || (ctx->input.length < mender_artifact_round_up(ctx->file.size, MENDER_ARTIFACT_STREAM_BLOCK_SIZE))) {
        return MENDER_OK;
    }

    /* Read header-info */
    if (NULL == (object = cJSON_ParseWithLength(ctx->input.data, ctx->file.size))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }
    cJSON *json_payloads = cJSON_GetObjectItemCaseSensitive(object, "payloads");
    if (true == cJSON_IsArray(json_payloads)) {
        ctx->payloads.size = cJSON_GetArraySize(json_payloads);
        if (NULL == (ctx->payloads.values = (mender_artifact_payload_t *)malloc(ctx->payloads.size * sizeof(mender_artifact_payload_t)))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        memset(ctx->payloads.values, 0, ctx->payloads.size * sizeof(mender_artifact_payload_t));
        size_t index        = 0;
        cJSON *json_payload = NULL;
        cJSON_ArrayForEach(json_payload, json_payloads) {
            if (true == cJSON_IsObject(json_payload)) {
                cJSON *json_payload_type = cJSON_GetObjectItemCaseSensitive(json_payload, "type");
                if (cJSON_IsString(json_payload_type)) {
                    if (NULL == (ctx->payloads.values[index].type = strdup(cJSON_GetStringValue(json_payload_type)))) {
                        mender_log_error("Unable to allocate memory");
                        ret = MENDER_FAIL;
                        goto END;
                    }
                } else {
                    mender_log_error("Invalid header-info file");
                    ret = MENDER_FAIL;
                    goto END;
                }
            } else {
                mender_log_error("Invalid header-info file");
                ret = MENDER_FAIL;
                goto END;
            }
            index++;
        }
    } else {
        mender_log_error("Invalid header-info file");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Shift data in the buffer */
    if (MENDER_OK != mender_artifact_shift_data(ctx, mender_artifact_round_up(ctx->file.size, MENDER_ARTIFACT_STREAM_BLOCK_SIZE))) {
        mender_log_error("Unable to shift input data");
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

static mender_err_t
mender_artifact_read_meta_data(mender_artifact_ctx_t *ctx) {

    assert(NULL != ctx);
    size_t index = 0;

    /* Retrieve payload index */
    if (1 != sscanf(ctx->file.name, "header.tar/headers/%u/meta-data", (unsigned int *)&index)) {
        mender_log_error("Invalid artifact format");
        return MENDER_FAIL;
    }
    if (index >= ctx->payloads.size) {
        mender_log_error("Invalid artifact format");
        return MENDER_FAIL;
    }

    /* Check size of the meta-data */
    if (0 == mender_artifact_round_up(ctx->file.size, MENDER_ARTIFACT_STREAM_BLOCK_SIZE)) {
        /* Nothing to do */
        return MENDER_DONE;
    }

    /* Check if all data have been received */
    if ((NULL == ctx->input.data) || (ctx->input.length < mender_artifact_round_up(ctx->file.size, MENDER_ARTIFACT_STREAM_BLOCK_SIZE))) {
        return MENDER_OK;
    }

    /* Read meta-data */
    if (NULL == (ctx->payloads.values[index].meta_data = cJSON_ParseWithLength(ctx->input.data, ctx->file.size))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }

    /* Shift data in the buffer */
    if (MENDER_OK != mender_artifact_shift_data(ctx, mender_artifact_round_up(ctx->file.size, MENDER_ARTIFACT_STREAM_BLOCK_SIZE))) {
        mender_log_error("Unable to shift input data");
        return MENDER_FAIL;
    }

    return MENDER_DONE;
}

static mender_err_t
mender_artifact_read_data(mender_artifact_ctx_t *ctx, mender_err_t (*callback)(char *, cJSON *, char *, size_t, void *, size_t, size_t)) {

    assert(NULL != ctx);
    assert(NULL != callback);
    size_t       index = 0;
    mender_err_t ret;

    /* Retrieve payload index */
    if (1 != sscanf(ctx->file.name, "data/%u.tar", (unsigned int *)&index)) {
        mender_log_error("Invalid artifact format");
        return MENDER_FAIL;
    }
    if (index >= ctx->payloads.size) {
        mender_log_error("Invalid artifact format");
        return MENDER_FAIL;
    }

    /* Check if a file name is provided (we don't check the extension because we don't know it) */
    if (strlen("data/xxxx.tar") == strlen(ctx->file.name)) {

        /* Beginning of the data file */
        if (MENDER_OK != (ret = callback(ctx->payloads.values[index].type, ctx->payloads.values[index].meta_data, NULL, 0, NULL, 0, 0))) {
            mender_log_error("An error occurred");
            return ret;
        }

        return MENDER_DONE;
    }

    /* Check size of the data */
    if (0 == mender_artifact_round_up(ctx->file.size, MENDER_ARTIFACT_STREAM_BLOCK_SIZE)) {
        /* Nothing to do */
        return MENDER_DONE;
    }

    /* Parse data until the end of the file has been reached */
    do {

        /* Check if enough data are received (at least one block) */
        if ((NULL == ctx->input.data) || (ctx->input.length < MENDER_ARTIFACT_STREAM_BLOCK_SIZE)) {
            return MENDER_OK;
        }

        /* Compute length */
        size_t length
            = ((ctx->file.size - ctx->file.index) > MENDER_ARTIFACT_STREAM_BLOCK_SIZE) ? MENDER_ARTIFACT_STREAM_BLOCK_SIZE : (ctx->file.size - ctx->file.index);

        /* Invoke callback */
        if (MENDER_OK
            != (ret = callback(ctx->payloads.values[index].type,
                               ctx->payloads.values[index].meta_data,
                               strstr(ctx->file.name, ".tar") + strlen(".tar") + 1,
                               ctx->file.size,
                               ctx->input.data,
                               ctx->file.index,
                               length))) {
            mender_log_error("An error occurred");
            return ret;
        }

        /* Update index */
        ctx->file.index += MENDER_ARTIFACT_STREAM_BLOCK_SIZE;

        /* Shift data in the buffer */
        if (MENDER_OK != (ret = mender_artifact_shift_data(ctx, MENDER_ARTIFACT_STREAM_BLOCK_SIZE))) {
            mender_log_error("Unable to shift input data");
            return ret;
        }

    } while (ctx->file.index < ctx->file.size);

    return MENDER_DONE;
}

static mender_err_t
mender_artifact_drop_file(mender_artifact_ctx_t *ctx) {

    assert(NULL != ctx);
    mender_err_t ret;

    /* Check size of the data */
    if (0 == mender_artifact_round_up(ctx->file.size, MENDER_ARTIFACT_STREAM_BLOCK_SIZE)) {
        /* Nothing to do */
        return MENDER_DONE;
    }

    /* Parse data until the end of the file has been reached */
    do {

        /* Check if enough data are received (at least one block) */
        if ((NULL == ctx->input.data) || (ctx->input.length < MENDER_ARTIFACT_STREAM_BLOCK_SIZE)) {
            return MENDER_OK;
        }

        /* Update index */
        ctx->file.index += MENDER_ARTIFACT_STREAM_BLOCK_SIZE;

        /* Shift data in the buffer */
        if (MENDER_OK != (ret = mender_artifact_shift_data(ctx, MENDER_ARTIFACT_STREAM_BLOCK_SIZE))) {
            mender_log_error("Unable to shift input data");
            return ret;
        }

    } while (ctx->file.index < ctx->file.size);

    return MENDER_DONE;
}

static mender_err_t
mender_artifact_shift_data(mender_artifact_ctx_t *ctx, size_t length) {

    assert(NULL != ctx);
    char *tmp;

    /* Shift data */
    if (length > 0) {
        if (ctx->input.length > length) {
            memcpy(ctx->input.data, (void *)(((uint8_t *)ctx->input.data) + length), ctx->input.length - length);
            if (NULL == (tmp = realloc(ctx->input.data, ctx->input.length - length))) {
                mender_log_error("Unable to allocate memory");
                return MENDER_FAIL;
            }
            ctx->input.data = tmp;
            ctx->input.length -= length;
        } else {
            free(ctx->input.data);
            ctx->input.data   = NULL;
            ctx->input.length = 0;
        }
    }

    return MENDER_OK;
}

static size_t
mender_artifact_round_up(size_t length, size_t incr) {
    return length + (incr - length % incr) % incr;
}
