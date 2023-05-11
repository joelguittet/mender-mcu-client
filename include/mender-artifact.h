/**
 * @file      mender-artifact.h
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

#ifndef __MENDER_ARTIFACT_H__
#define __MENDER_ARTIFACT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <cJSON.h>
#include "mender-common.h"

/**
 * @brief Artifact state machine used to process input data stream
 */
typedef enum {
    MENDER_ARTIFACT_STREAM_STATE_PARSING_HEADER = 0, /**< Currently parsing header */
    MENDER_ARTIFACT_STREAM_STATE_PARSING_DATA        /**< Currently parsing data */
} mender_artifact_stream_state_t;

/**
 * @brief Artifact payloads
 */
typedef struct {
    char * type;      /**< Type of the payload */
    cJSON *meta_data; /**< Meta-data from the header tarball, NULL if no meta-data */
} mender_artifact_payload_t;

/**
 * @brief Artifact context
 */
typedef struct {
    mender_artifact_stream_state_t stream_state; /**< Stream state of the artifact processing */
    struct {
        void * data;   /**< Data received, concatenated chunk by chunk */
        size_t length; /**< Length of the data received */
    } input;           /**< Input data of the artifact */
    struct {
        size_t                     size;   /**< Number of payloads in the artifact */
        mender_artifact_payload_t *values; /**< Values of payloads in the artifact */
    } payloads;                            /**< Payloads of the artifact */
    struct {
        char * name;  /**< Name of the file currently parsed */
        size_t size;  /**< Size of the file currently parsed (bytes) */
        size_t index; /**< Index of the data in the file currently parsed (bytes), incremented block by block */
    } file;           /**< Information about the file currently parsed */
} mender_artifact_ctx_t;

/**
 * @brief Function used to create a new artifact context
 * @return Artifact context if the function succeeds, NULL otherwise
 */
mender_artifact_ctx_t *mender_artifact_create_ctx(void);

/**
 * @brief Function used to process data from artifact stream
 * @param ctx Artifact context
 * @param input_data Input data from the stream
 * @param input_length Length of the input data from the stream
 * @param callback Callback function to be invoked to perform the treatment of the data from the artifact
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_artifact_process_data(mender_artifact_ctx_t *ctx,
                                          void *                 input_data,
                                          size_t                 input_length,
                                          mender_err_t (*callback)(char *, cJSON *, char *, size_t, void *, size_t, size_t));

/**
 * @brief Function used to release artifact context
 * @param ctx Artifact context
 */
void mender_artifact_release_ctx(mender_artifact_ctx_t *ctx);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_ARTIFACT_H__ */
