/**
 * @file      mender-artifact.h
 * @brief     Mender artifact parser
 *
 * Copyright joelguittet and mender-mcu-client contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __MENDER_ARTIFACT_H__
#define __MENDER_ARTIFACT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-utils.h"

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
    char  *type;      /**< Type of the payload */
    cJSON *meta_data; /**< Meta-data from the header tarball, NULL if no meta-data */
} mender_artifact_payload_t;

/**
 * @brief Artifact context
 */
typedef struct {
    mender_artifact_stream_state_t stream_state; /**< Stream state of the artifact processing */
    struct {
        void  *data;   /**< Data received, concatenated chunk by chunk */
        size_t length; /**< Length of the data received */
    } input;           /**< Input data of the artifact */
    struct {
        size_t                     size;   /**< Number of payloads in the artifact */
        mender_artifact_payload_t *values; /**< Values of payloads in the artifact */
    } payloads;                            /**< Payloads of the artifact */
    struct {
        char  *name;  /**< Name of the file currently parsed */
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
                                          void                  *input_data,
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
