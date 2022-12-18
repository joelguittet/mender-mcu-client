/**
 * @file      mender-untar.h
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

#ifndef __MENDER_UNTAR_H__
#define __MENDER_UNTAR_H__

#include "mender-common.h"

/**
 * @brief un-tar state machine used to process input data stream
 */
typedef enum {
    UNTAR_STREAM_STATE_PARSING_HEADER = 0, /**< Currently parsing header */
    UNTAR_STREAM_STATE_PARSING_FILE        /**< Currently parsing file content */
} mender_untar_state_t;

/**
 * @brief un-tar context
 */
typedef struct {
    void *               data;        /**< Data received, concatenated chunk by chunk */
    size_t               data_length; /**< Length of the data received */
    mender_untar_state_t state;       /**< Current state of the un-tar processing */
    struct {
        size_t size;  /**< Size of the file currently parsed (bytes) */
        size_t index; /**< Index of the file currently parsed (bytes), incremented block by block */
    } current_file;
} mender_untar_ctx_t;

/**
 * @brief un-tar header info
 */
typedef struct {
    char   name[100]; /**< Name of the file */
    size_t size;      /**< Size of the file (bytes) */
} mender_untar_header_t;

/**
 * @brief Function used to create a new un-tar context
 * @return un-tar context if the function succeeds, NULL otherwise
 */
mender_untar_ctx_t *mender_untar_init(void);

/**
 * @brief Function used to parse data from tar stream
 * @param ctx un-tar context
 * @param input_data Input data from the stream
 * @param input_length Length of the input data from the stream
 * @param header Header of the next file, NULL if waiting for the next input data from the stream
 * @param output_data Output data
 * @param output_length Output data length
 * @return 1 if the function should be called again to get more data, 0 if data have been parsed, -1 if an error occurred
 */
int mender_untar_parse(
    mender_untar_ctx_t *ctx, void *input_data, size_t input_length, mender_untar_header_t **header, void **output_data, size_t *output_length);

/**
 * @brief Function used to create release un-tar context
 * @param ctx un-tar context
 */
void mender_untar_release(mender_untar_ctx_t *ctx);

#endif /* __MENDER_UNTAR_H__ */
