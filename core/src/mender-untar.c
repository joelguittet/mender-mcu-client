/**
 * @file      mender-untar.c
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

#include "mender-untar.h"

/**
 * @brief tar block size
 */
#define UNTAR_STREAM_BLOCK_SIZE 512

/**
 * @brief tar file header
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
} mender_untar_tar_header_t;

/**
 * @brief Parse header of tar file
 * @param ctx un-tar context
 * @param header Header of the next file
 * @return 0 if the function succeeds, -1 otherwise
 */
static int mender_untar_parse_header(mender_untar_ctx_t *ctx, mender_untar_header_t **header);

/**
 * @brief Parse file of tar file
 * @param ctx un-tar context
 * @param output_data File data
 * @param output_length File data length
 * @return 0 if the function succeeds, -1 otherwise
 */
static int mender_untar_parse_file(mender_untar_ctx_t *ctx, void **output_data, size_t *output_length);

/**
 * @brief Shift remaining data after parsing header or file
 * @param ctx un-tar context
 * @return 1 if the function should be called again to get more data, 0 if data have been parsed, -1 if an error occurred
 */
static int mender_untar_shift_data(mender_untar_ctx_t *ctx);

mender_untar_ctx_t *
mender_untar_init(void) {

    mender_untar_ctx_t *ctx;

    /* Create new context */
    if (NULL == (ctx = malloc(sizeof(mender_untar_ctx_t)))) {
        return NULL;
    }
    memset(ctx, 0, sizeof(mender_untar_ctx_t));

    return ctx;
}

int
mender_untar_parse(mender_untar_ctx_t *ctx, void *input_data, size_t input_length, mender_untar_header_t **header, void **output_data, size_t *output_length) {

    assert(NULL != ctx);
    assert(NULL != header);
    assert(NULL != output_data);
    assert(NULL != output_length);
    void *tmp;
    int   res;

    /* Copy data to the end of the internal buffer */
    if ((NULL != input_data) && (0 != input_length)) {
        if (NULL == (tmp = realloc(ctx->data, ctx->data_length + input_length))) {
            /* Unbale to allocate memory */
            return -1;
        }
        ctx->data = tmp;
        memcpy(ctx->data + ctx->data_length, input_data, input_length);
        ctx->data_length += input_length;
    }

    /* Check if enought data are received */
    if (ctx->data_length < UNTAR_STREAM_BLOCK_SIZE) {
        return 0;
    }

    /* Treatment depending of the state machine */
    switch (ctx->state) {
        case UNTAR_STREAM_STATE_PARSING_HEADER:
            /* Parse header */
            if (0 == (res = mender_untar_parse_header(ctx, header))) {
                res = mender_untar_shift_data(ctx);
            }
            break;
        case UNTAR_STREAM_STATE_PARSING_FILE:
            /* Parse file */
            if (0 == (res = mender_untar_parse_file(ctx, output_data, output_length))) {
                res = mender_untar_shift_data(ctx);
            }
            break;
        default:
            /* Should not occur */
            res = -1;
            break;
    }

    return res;
}

void
mender_untar_release(mender_untar_ctx_t *ctx) {

    /* Release memory */
    if (NULL != ctx) {
        if (NULL != ctx->data) {
            free(ctx->data);
        }
        free(ctx);
    }
}

static int
mender_untar_parse_header(mender_untar_ctx_t *ctx, mender_untar_header_t **header) {

    assert(NULL != ctx);
    assert(NULL != ctx->data);
    assert(NULL != header);

    /* Cast block to tar header structure */
    mender_untar_tar_header_t *tar_header = (mender_untar_tar_header_t *)ctx->data;

    /* Check if the end of the current tar file is reached */
    if ('\0' == tar_header->name[0]) {
        return 0;
    }

    /* Check magic */
    if (strncmp(tar_header->magic, "ustar", strlen("ustar"))) {
        /* Invalid magic */
        return -1;
    }

    /* Check if the file is a new tar file */
    if (!strncmp(tar_header->name + strlen(tar_header->name) - strlen(".tar"), ".tar", strlen(".tar"))) {
        return 0;
    }

    /* Create new header info */
    if (NULL == (*header = malloc(sizeof(mender_untar_header_t)))) {
        /* Unable to allocate memory */
        return -1;
    }

    /* Copy wanted info */
    memcpy((*header)->name, tar_header->name, 100);
    sscanf(tar_header->size, "%o", (unsigned int *)&((*header)->size));

    /* Update state machine */
    ctx->state             = UNTAR_STREAM_STATE_PARSING_FILE;
    ctx->current_file.size = (*header)->size;

    return 0;
}

static int
mender_untar_parse_file(mender_untar_ctx_t *ctx, void **output_data, size_t *output_length) {

    assert(NULL != ctx);
    assert(NULL != ctx->data);
    assert(NULL != output_data);
    assert(NULL != output_length);

    /* Compute length to be copied */
    if (0
        != (*output_length = ((ctx->current_file.size - ctx->current_file.index) > UNTAR_STREAM_BLOCK_SIZE)
                                 ? UNTAR_STREAM_BLOCK_SIZE
                                 : (ctx->current_file.size - ctx->current_file.index))) {

        /* Create new file data block */
        if (NULL == (*output_data = malloc(*output_length))) {
            /* Unable to allocate memory */
            return -1;
        }

        /* Copy file data */
        memcpy(*output_data, ctx->data, *output_length);
    }

    /* Update state machine */
    ctx->current_file.index += *output_length;
    if (ctx->current_file.index >= ctx->current_file.size) {
        ctx->state              = UNTAR_STREAM_STATE_PARSING_HEADER;
        ctx->current_file.size  = 0;
        ctx->current_file.index = 0;
    }

    return 0;
}

static int
mender_untar_shift_data(mender_untar_ctx_t *ctx) {

    assert(NULL != ctx);
    char *tmp;

    /* Shift remaining data */
    if (ctx->data_length > UNTAR_STREAM_BLOCK_SIZE) {
        memcpy(ctx->data, ctx->data + UNTAR_STREAM_BLOCK_SIZE, ctx->data_length - UNTAR_STREAM_BLOCK_SIZE);
        if (NULL == (tmp = realloc(ctx->data, ctx->data_length - UNTAR_STREAM_BLOCK_SIZE))) {
            /* Unable to allocate memory */
            return -1;
        }
        ctx->data = tmp;
        ctx->data_length -= UNTAR_STREAM_BLOCK_SIZE;
    } else {
        free(ctx->data);
        ctx->data        = NULL;
        ctx->data_length = 0;
    }

    return (ctx->data_length >= UNTAR_STREAM_BLOCK_SIZE) ? 1 : 0;
}
