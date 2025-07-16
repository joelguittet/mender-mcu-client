/**
 * @file      mender-troubleshoot-file-transfer.c
 * @brief     Mender MCU Troubleshoot add-on file transfer message handler implementation
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

#include "mender-log.h"
#include "mender-troubleshoot-api.h"
#include "mender-troubleshoot-file-transfer.h"
#include "mender-troubleshoot-msgpack.h"

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT
#ifdef CONFIG_MENDER_CLIENT_TROUBLESHOOT_FILE_TRANSFER

/**
 * @brief Message type
 */
#define MENDER_TROUBLESHOOT_FILE_TRANSFER_MESSAGE_TYPE_GET       "get_file"
#define MENDER_TROUBLESHOOT_FILE_TRANSFER_MESSAGE_TYPE_PUT       "put_file"
#define MENDER_TROUBLESHOOT_FILE_TRANSFER_MESSAGE_TYPE_ACK       "ack"
#define MENDER_TROUBLESHOOT_FILE_TRANSFER_MESSAGE_TYPE_STAT      "stat"
#define MENDER_TROUBLESHOOT_FILE_TRANSFER_MESSAGE_TYPE_FILE_INFO "file_info"
#define MENDER_TROUBLESHOOT_FILE_TRANSFER_MESSAGE_TYPE_CHUNK     "file_chunk"
#define MENDER_TROUBLESHOOT_FILE_TRANSFER_MESSAGE_TYPE_ERROR     "error"

/**
 * @brief Chunk size
 */
#define MENDER_TROUBLESHOOT_FILE_TRANSFER_CHUNK_SIZE 1024

/**
 * @brief Number of chunks packets to send/receive before waiting/sending an ack
 */
#define MENDER_TROUBLESHOOT_FILE_TRANSFER_CHUNK_PACKETS 10

/**
 * @brief Get file
 */
typedef struct {
    char *path; /**< Source path */
} mender_troubleshoot_file_transfer_get_file_t;

/**
 * @brief Upload request
 */
typedef struct {
    char *src_path; /**< Source path */
    char *path;     /**< Destination path */
} mender_troubleshoot_file_transfer_upload_request_t;

/**
 * @brief Stat file
 */
typedef struct {
    char *path; /**< Path */
} mender_troubleshoot_file_transfer_stat_file_t;

/**
 * @brief File info
 */
typedef struct {
    char     *path; /**< Path */
    size_t   *size; /**< Size */
    uint32_t *uid;  /**< UID */
    uint32_t *gid;  /**< GID */
    uint32_t *mode; /**< Mode */
    time_t   *time; /**< Last modification time, seconds since epoch */
} mender_troubleshoot_file_transfer_file_info_t;

/**
 * @brief Error
 */
typedef struct {
    char *description; /**< Description */
    char *type;        /**< Type */
    char *id;          /**< ID */
} mender_troubleshoot_file_transfer_error_t;

/**
 * @brief State machine used for sending files to the server
 */
enum {
    MENDER_TROUBLESHOOT_FILE_TRANSFER_IDLE,    /**< Idle */
    MENDER_TROUBLESHOOT_FILE_TRANSFER_READING, /**< Reading file and sending chunks to the server */
    MENDER_TROUBLESHOOT_FILE_TRANSFER_EOF      /**< Waiting for the latest ack to close the handle */
} mender_troubleshoot_file_transfer_state_machine
    = MENDER_TROUBLESHOOT_FILE_TRANSFER_IDLE;

/**
 * @brief Mender troubleshoot file transfer callbacks
 */
static mender_troubleshoot_file_transfer_callbacks_t mender_troubleshoot_file_transfer_callbacks;

/**
 * @brief File handle used to store temporary reference to read/write files
 */
static void *mender_troubleshoot_file_transfer_handle = NULL;

/**
 * @brief Function called to perform the treatment of the file transfer get messages
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_file_transfer_get_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response);

/**
 * @brief Function called to perform the treatment of the file transfer put messages
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_file_transfer_put_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response);

/**
 * @brief Function called to perform the treatment of the file transfer ack messages
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_file_transfer_ack_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response);

/**
 * @brief Function called to perform the treatment of the file transfer stat messages
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_file_transfer_stat_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response);

/**
 * @brief Function called to perform the treatment of the file transfer chunk messages
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_file_transfer_chunk_message_handler(mender_troubleshoot_protomsg_t  *protomsg,
                                                                            mender_troubleshoot_protomsg_t **response);

/**
 * @brief Function called to perform the treatment of the file transfer error messages
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_file_transfer_error_message_handler(mender_troubleshoot_protomsg_t  *protomsg,
                                                                            mender_troubleshoot_protomsg_t **response);

/**
 * @brief Unpack and decode get file
 * @param data Packed data to be decoded
 * @param length Length of the data to be decoded
 * @return Get file if the function succeeds, NULL otherwise
 */
static mender_troubleshoot_file_transfer_get_file_t *mender_troubleshoot_file_transfer_get_file_unpack(void *data, size_t length);

/**
 * @brief Decode get file object
 * @param object Get file object
 * @return Get file if the function succeeds, NULL otherwise
 */
static mender_troubleshoot_file_transfer_get_file_t *mender_troubleshoot_file_transfer_get_file_decode(msgpack_object *object);

/**
 * @brief Release get file
 * @param get_file Get file
 */
static void mender_troubleshoot_file_transfer_get_file_release(mender_troubleshoot_file_transfer_get_file_t *get_file);

/**
 * @brief Unpack and decode upload request
 * @param data Packed data to be decoded
 * @param length Length of the data to be decoded
 * @return Upload request if the function succeeds, NULL otherwise
 */
static mender_troubleshoot_file_transfer_upload_request_t *mender_troubleshoot_file_transfer_upload_request_unpack(void *data, size_t length);

/**
 * @brief Decode upload request object
 * @param object Upload request object
 * @return Upload request if the function succeeds, NULL otherwise
 */
static mender_troubleshoot_file_transfer_upload_request_t *mender_troubleshoot_file_transfer_upload_request_decode(msgpack_object *object);

/**
 * @brief Release upload request
 * @param upload_request Upload request
 */
static void mender_troubleshoot_file_transfer_upload_request_release(mender_troubleshoot_file_transfer_upload_request_t *upload_request);

/**
 * @brief Unpack and decode stat file
 * @param data Packed data to be decoded
 * @param length Length of the data to be decoded
 * @return Fiel stat if the function succeeds, NULL otherwise
 */
static mender_troubleshoot_file_transfer_stat_file_t *mender_troubleshoot_file_transfer_stat_file_unpack(void *data, size_t length);

/**
 * @brief Decode stat file object
 * @param object Fiel stat object
 * @return Upload request if the function succeeds, NULL otherwise
 */
static mender_troubleshoot_file_transfer_stat_file_t *mender_troubleshoot_file_transfer_stat_file_decode(msgpack_object *object);

/**
 * @brief Release stat file
 * @param stat_file Stat file
 */
static void mender_troubleshoot_file_transfer_stat_file_release(mender_troubleshoot_file_transfer_stat_file_t *stat_file);

/**
 * @brief Encode and pack file info
 * @param file_info File info
 * @param data Packed data encoded
 * @param length Length of the data encoded
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_troubleshoot_file_transfer_file_info_pack(mender_troubleshoot_file_transfer_file_info_t *file_info, void **data, size_t *length);

/**
 * @brief Encode file info
 * @param file_info File info
 * @param data Packed data encoded
 * @param length Length of the data encoded
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_troubleshoot_file_transfer_file_info_encode(mender_troubleshoot_file_transfer_file_info_t *file_info, msgpack_object *object);

/**
 * @brief Release file info
 * @param file_info File info
 */
static void mender_troubleshoot_file_transfer_file_info_release(mender_troubleshoot_file_transfer_file_info_t *file_info);

/**
 * @brief Function used to format file transfer file info message
 * @param protomsg Received proto message
 * @param file_info File info
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_file_transfer_format_file_info(mender_troubleshoot_protomsg_t                *protomsg,
                                                                       mender_troubleshoot_file_transfer_file_info_t *file_info,
                                                                       mender_troubleshoot_protomsg_t               **response);

/**
 * @brief Function used to format file transfer ack message
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_file_transfer_format_ack(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response);

/**
 * @brief Function used to format file transfer error message
 * @param protomsg Received proto message
 * @param error Error
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_file_transfer_format_error(mender_troubleshoot_protomsg_t            *protomsg,
                                                                   mender_troubleshoot_file_transfer_error_t *error,
                                                                   mender_troubleshoot_protomsg_t           **response);

/**
 * @brief Encode and pack error message
 * @param error Error
 * @param data Packed data encoded
 * @param length Length of the data encoded
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_troubleshoot_file_transfer_error_message_pack(mender_troubleshoot_file_transfer_error_t *error, void **data, size_t *length);

/**
 * @brief Encode error message
 * @param error Error
 * @param data Packed data encoded
 * @param length Length of the data encoded
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_troubleshoot_file_transfer_error_message_encode(mender_troubleshoot_file_transfer_error_t *error, msgpack_object *object);

/**
 * @brief Function called to send file transfer chunk protomsg
 * @param sid Session ID from the server
 * @param user_id User ID from the server
 * @param offset Offset of data sent to the server
 * @param data Data to send to the server
 * @param length Length of the data to send to the server
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_file_transfer_send_chunk(char *sid, char *user_id, size_t offset, void *data, size_t length);

mender_err_t
mender_troubleshoot_file_transfer_init(mender_troubleshoot_file_transfer_callbacks_t *callbacks) {

    /* Save callbacks */
    if (NULL != callbacks) {
        memcpy(&mender_troubleshoot_file_transfer_callbacks, callbacks, sizeof(mender_troubleshoot_file_transfer_callbacks_t));
    }

    return MENDER_OK;
}

mender_err_t
mender_troubleshoot_file_transfer_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    assert(NULL != protomsg->hdr);
    mender_err_t ret = MENDER_OK;

    /* Verify integrity of the message */
    if (NULL == protomsg->hdr->typ) {
        mender_log_error("Invalid message received");
        ret = MENDER_FAIL;
        goto FAIL;
    }

    /* Treatment of the message depending of the message type */
    if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_FILE_TRANSFER_MESSAGE_TYPE_GET)) {
        /* Open file for reading */
        ret = mender_troubleshoot_file_transfer_get_message_handler(protomsg, response);
    } else if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_FILE_TRANSFER_MESSAGE_TYPE_PUT)) {
        /* Open file for writing */
        ret = mender_troubleshoot_file_transfer_put_message_handler(protomsg, response);
    } else if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_FILE_TRANSFER_MESSAGE_TYPE_ACK)) {
        /* Continue reading file */
        ret = mender_troubleshoot_file_transfer_ack_message_handler(protomsg, response);
    } else if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_FILE_TRANSFER_MESSAGE_TYPE_STAT)) {
        /* Get file statistics */
        ret = mender_troubleshoot_file_transfer_stat_message_handler(protomsg, response);
    } else if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_FILE_TRANSFER_MESSAGE_TYPE_FILE_INFO)) {
        /* Nothing to do */
    } else if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_FILE_TRANSFER_MESSAGE_TYPE_CHUNK)) {
        /* Write data to the file */
        ret = mender_troubleshoot_file_transfer_chunk_message_handler(protomsg, response);
    } else if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_FILE_TRANSFER_MESSAGE_TYPE_ERROR)) {
        /* Close file */
        ret = mender_troubleshoot_file_transfer_error_message_handler(protomsg, response);
    } else {
        /* Not supported */
        mender_log_error("Unsupported message received with message type '%s'", protomsg->hdr->typ);
        ret = MENDER_FAIL;
        goto FAIL;
    }

FAIL:

    return ret;
}

mender_err_t
mender_troubleshoot_file_transfer_exit(void) {

    /* Nothing to do */
    return MENDER_OK;
}

static mender_err_t
mender_troubleshoot_file_transfer_get_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    mender_troubleshoot_file_transfer_error_t     error;
    mender_troubleshoot_file_transfer_get_file_t *get_file = NULL;
    uint8_t                                      *data     = NULL;
    size_t                                        length;
    size_t                                        offset;
    int                                           chunk_index = 0;
    mender_err_t                                  ret         = MENDER_OK;

    /* Initialize error message */
    memset(&error, 0, sizeof(mender_troubleshoot_file_transfer_error_t));

    /* Verify integrity of the message */
    if (NULL == protomsg->hdr) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->hdr->sid) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->hdr->properties) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->hdr->properties->user_id) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->body) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->body->data) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }

    /* Unpack and decode data */
    if (NULL == (get_file = mender_troubleshoot_file_transfer_get_file_unpack(protomsg->body->data, protomsg->body->length))) {
        mender_log_error("Unable to decode upload request");
        error.description = "Unable to decode upload request";
        ret               = MENDER_FAIL;
        goto FAIL;
    }

    /* Verify integrity of the get file */
    if (NULL == get_file->path) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }

    /* Treatment depending of the state machine */
    if (MENDER_TROUBLESHOOT_FILE_TRANSFER_IDLE == mender_troubleshoot_file_transfer_state_machine) {

        /* Open file for reading */
        if (NULL != mender_troubleshoot_file_transfer_callbacks.open) {
            if (MENDER_OK != (ret = mender_troubleshoot_file_transfer_callbacks.open(get_file->path, "rb", &mender_troubleshoot_file_transfer_handle))) {
                mender_log_error("Unable to open file '%s' for reading", get_file->path);
                error.description = "Unable to open file for reading";
                goto FAIL;
            }
        }
        mender_troubleshoot_file_transfer_state_machine = MENDER_TROUBLESHOOT_FILE_TRANSFER_READING;
    }
    /* Intentional pass-through */
    if (MENDER_TROUBLESHOOT_FILE_TRANSFER_READING == mender_troubleshoot_file_transfer_state_machine) {

        /* Read and send file chunk by chunk */
        if (NULL == (data = (uint8_t *)malloc(MENDER_TROUBLESHOOT_FILE_TRANSFER_CHUNK_SIZE))) {
            mender_log_error("Unable to allocate memory");
            error.description = "Internal error";
            goto FAIL;
        }
        offset = 0;
        do {
            length = MENDER_TROUBLESHOOT_FILE_TRANSFER_CHUNK_SIZE;
            if (NULL != mender_troubleshoot_file_transfer_callbacks.read) {
                if (MENDER_OK != (ret = mender_troubleshoot_file_transfer_callbacks.read(mender_troubleshoot_file_transfer_handle, data, &length))) {
                    mender_log_error("Unable to read file");
                    error.description = "Unable to read file";
                    goto FAIL;
                }
            }
            if (MENDER_OK
                != (ret = mender_troubleshoot_file_transfer_send_chunk(protomsg->hdr->sid, protomsg->hdr->properties->user_id, offset, data, length))) {
                mender_log_error("Unable to send chunk");
                error.description = "Unable to send file";
                goto FAIL;
            }
            offset += length;
            chunk_index++;
        } while ((chunk_index < MENDER_TROUBLESHOOT_FILE_TRANSFER_CHUNK_PACKETS) && (length > 0));

        /* Check if the end of the file has been reached */
        if (0 == length) {
            mender_troubleshoot_file_transfer_state_machine = MENDER_TROUBLESHOOT_FILE_TRANSFER_EOF;
        }
    }

    /* Release memory */
    mender_troubleshoot_file_transfer_get_file_release(get_file);
    if (NULL != data) {
        free(data);
    }

    return ret;

FAIL:

    /* Format error */
    if (MENDER_OK != mender_troubleshoot_file_transfer_format_error(protomsg, &error, response)) {
        mender_log_error("Unable to format response");
    }

    /* Release memory */
    mender_troubleshoot_file_transfer_get_file_release(get_file);
    if (NULL != data) {
        free(data);
    }

    return ret;
}

static mender_err_t
mender_troubleshoot_file_transfer_put_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    mender_troubleshoot_file_transfer_error_t           error;
    mender_troubleshoot_file_transfer_upload_request_t *upload_request = NULL;
    mender_err_t                                        ret            = MENDER_OK;

    /* Initialize error message */
    memset(&error, 0, sizeof(mender_troubleshoot_file_transfer_error_t));

    /* Verify integrity of the message */
    if (NULL == protomsg->body) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->body->data) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }

    /* Unpack and decode data */
    if (NULL == (upload_request = mender_troubleshoot_file_transfer_upload_request_unpack(protomsg->body->data, protomsg->body->length))) {
        mender_log_error("Unable to decode upload request");
        error.description = "Unable to decode upload request";
        ret               = MENDER_FAIL;
        goto FAIL;
    }

    /* Verify integrity of the upload request */
    if (NULL == upload_request->path) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }

    /* Open file for writing */
    if (NULL != mender_troubleshoot_file_transfer_callbacks.open) {
        if (MENDER_OK != (ret = mender_troubleshoot_file_transfer_callbacks.open(upload_request->path, "wb", &mender_troubleshoot_file_transfer_handle))) {
            mender_log_error("Unable to open file '%s' for writing", upload_request->path);
            error.description = "Unable to open file for writing";
            goto FAIL;
        }
    }

    /* Format ack */
    if (MENDER_OK != (ret = mender_troubleshoot_file_transfer_format_ack(protomsg, response))) {
        mender_log_error("Unable to format response");
        error.description = "Unable to format response";
        goto FAIL;
    }

    /* Release memory */
    mender_troubleshoot_file_transfer_upload_request_release(upload_request);

    return ret;

FAIL:

    /* Format error */
    if (MENDER_OK != mender_troubleshoot_file_transfer_format_error(protomsg, &error, response)) {
        mender_log_error("Unable to format response");
    }

    /* Release memory */
    mender_troubleshoot_file_transfer_upload_request_release(upload_request);

    return ret;
}

static mender_err_t
mender_troubleshoot_file_transfer_ack_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    mender_troubleshoot_file_transfer_error_t error;
    uint8_t                                  *data = NULL;
    size_t                                    length;
    size_t                                    offset;
    int                                       chunk_index = 0;
    mender_err_t                              ret         = MENDER_OK;

    /* Initialize error message */
    memset(&error, 0, sizeof(mender_troubleshoot_file_transfer_error_t));

    /* Verify integrity of the message */
    if (NULL == protomsg->hdr) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->hdr->sid) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->hdr->properties) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->hdr->properties->user_id) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->hdr->properties->offset) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }

    /* Treatment depending of the state machine */
    if (MENDER_TROUBLESHOOT_FILE_TRANSFER_READING == mender_troubleshoot_file_transfer_state_machine) {

        /* Read and send file chunk by chunk */
        if (NULL == (data = (uint8_t *)malloc(MENDER_TROUBLESHOOT_FILE_TRANSFER_CHUNK_SIZE))) {
            mender_log_error("Unable to allocate memory");
            error.description = "Internal error";
            goto FAIL;
        }
        offset = *protomsg->hdr->properties->offset;
        do {
            length = MENDER_TROUBLESHOOT_FILE_TRANSFER_CHUNK_SIZE;
            if (NULL != mender_troubleshoot_file_transfer_callbacks.read) {
                if (MENDER_OK != (ret = mender_troubleshoot_file_transfer_callbacks.read(mender_troubleshoot_file_transfer_handle, data, &length))) {
                    mender_log_error("Unable to read file");
                    error.description = "Unable to read file";
                    goto FAIL;
                }
            }
            if (MENDER_OK
                != (ret = mender_troubleshoot_file_transfer_send_chunk(protomsg->hdr->sid, protomsg->hdr->properties->user_id, offset, data, length))) {
                mender_log_error("Unable to send chunk");
                error.description = "Unable to send file";
                goto FAIL;
            }
            offset += length;
            chunk_index++;
        } while ((chunk_index < MENDER_TROUBLESHOOT_FILE_TRANSFER_CHUNK_PACKETS) && (length > 0));

        /* Check if the end of the file has been reached */
        if (0 == length) {
            mender_troubleshoot_file_transfer_state_machine = MENDER_TROUBLESHOOT_FILE_TRANSFER_EOF;
        }

    } else if (MENDER_TROUBLESHOOT_FILE_TRANSFER_EOF == mender_troubleshoot_file_transfer_state_machine) {

        /* Close file */
        if (NULL != mender_troubleshoot_file_transfer_callbacks.close) {
            if (MENDER_OK != (ret = mender_troubleshoot_file_transfer_callbacks.close(mender_troubleshoot_file_transfer_handle))) {
                mender_log_error("Unable to close the file");
            }
        }
        mender_troubleshoot_file_transfer_state_machine = MENDER_TROUBLESHOOT_FILE_TRANSFER_IDLE;

    } else {

        /* Should not append */
        mender_log_error("An error occured");
        error.description                               = "Internal error";
        mender_troubleshoot_file_transfer_state_machine = MENDER_TROUBLESHOOT_FILE_TRANSFER_IDLE;
        goto FAIL;
    }

    /* Release memory */
    if (NULL != data) {
        free(data);
    }

    return ret;

FAIL:

    /* Format error */
    if (MENDER_OK != mender_troubleshoot_file_transfer_format_error(protomsg, &error, response)) {
        mender_log_error("Unable to format response");
    }

    /* Release memory */
    if (NULL != data) {
        free(data);
    }

    return ret;
}

static mender_err_t
mender_troubleshoot_file_transfer_stat_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    mender_troubleshoot_file_transfer_error_t      error;
    mender_troubleshoot_file_transfer_stat_file_t *stat_file = NULL;
    mender_troubleshoot_file_transfer_file_info_t *file_info = NULL;
    mender_err_t                                   ret       = MENDER_OK;

    /* Initialize error message */
    memset(&error, 0, sizeof(mender_troubleshoot_file_transfer_error_t));

    /* Verify integrity of the message */
    if (NULL == protomsg->body) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->body->data) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }

    /* Unpack and decode data */
    if (NULL == (stat_file = mender_troubleshoot_file_transfer_stat_file_unpack(protomsg->body->data, protomsg->body->length))) {
        mender_log_error("Unable to decode stat file");
        error.description = "Unable to decode stat file";
        ret               = MENDER_FAIL;
        goto FAIL;
    }

    /* Verify integrity of the stat file */
    if (NULL == stat_file->path) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }

    /* Prepare file info */
    if (NULL == (file_info = (mender_troubleshoot_file_transfer_file_info_t *)malloc(sizeof(mender_troubleshoot_file_transfer_file_info_t)))) {
        mender_log_error("Unable to allocate memory");
        error.description = "Internal error";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    memset(file_info, 0, sizeof(mender_troubleshoot_file_transfer_file_info_t));
    if (NULL == (file_info->path = strdup(stat_file->path))) {
        mender_log_error("Unable to allocate memory");
        error.description = "Internal error";
        ret               = MENDER_FAIL;
        goto FAIL;
    }

    /* Get statistics of the file */
    if (NULL != mender_troubleshoot_file_transfer_callbacks.stat) {
        if (MENDER_OK
            != (ret = mender_troubleshoot_file_transfer_callbacks.stat(
                    stat_file->path, &(file_info->size), &(file_info->uid), &(file_info->gid), &(file_info->mode), &(file_info->time)))) {
            mender_log_error("Unable to get statistics of the file '%s'", stat_file->path);
            error.description = "Unable to get statistics of the file";
            goto FAIL;
        }
    }

    /* Format file info */
    if (MENDER_OK != (ret = mender_troubleshoot_file_transfer_format_file_info(protomsg, file_info, response))) {
        mender_log_error("Unable to format response");
        error.description = "Internal error";
        goto FAIL;
    }

    /* Release memory */
    mender_troubleshoot_file_transfer_stat_file_release(stat_file);
    mender_troubleshoot_file_transfer_file_info_release(file_info);

    return ret;

FAIL:

    /* Format error */
    if (MENDER_OK != mender_troubleshoot_file_transfer_format_error(protomsg, &error, response)) {
        mender_log_error("Unable to format response");
    }

    /* Release memory */
    mender_troubleshoot_file_transfer_stat_file_release(stat_file);
    mender_troubleshoot_file_transfer_file_info_release(file_info);

    return ret;
}

static mender_err_t
mender_troubleshoot_file_transfer_chunk_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    mender_troubleshoot_file_transfer_error_t error;
    static int                                chunk_index = 0;
    mender_err_t                              ret         = MENDER_OK;

    /* Initialize error message */
    memset(&error, 0, sizeof(mender_troubleshoot_file_transfer_error_t));

    /* Check if data are provided */
    if (NULL != protomsg->body) {

        /* Write to the file */
        if (NULL != mender_troubleshoot_file_transfer_callbacks.write) {
            if (MENDER_OK
                != (ret = mender_troubleshoot_file_transfer_callbacks.write(
                        mender_troubleshoot_file_transfer_handle, protomsg->body->data, protomsg->body->length))) {
                mender_log_error("Unable to write to the file");
                error.description = "Unable to write to the file";
                goto FAIL;
            }
        }
        chunk_index++;

    } else {

        /* Close file */
        if (NULL != mender_troubleshoot_file_transfer_callbacks.close) {
            if (MENDER_OK != (ret = mender_troubleshoot_file_transfer_callbacks.close(mender_troubleshoot_file_transfer_handle))) {
                mender_log_error("Unable to close the file");
                error.description = "Unable to close the file";
            }
        }
    }

    /* Check if ack must be sent */
    if ((chunk_index >= MENDER_TROUBLESHOOT_FILE_TRANSFER_CHUNK_PACKETS) || (NULL == protomsg->body)) {
        /* Format ack */
        if (MENDER_OK != (ret = mender_troubleshoot_file_transfer_format_ack(protomsg, response))) {
            mender_log_error("Unable to format response");
            error.description = "Internal error";
            goto FAIL;
        }
        chunk_index = 0;
    }

    return ret;

FAIL:

    /* Format error */
    if (MENDER_OK != mender_troubleshoot_file_transfer_format_error(protomsg, &error, response)) {
        mender_log_error("Unable to format response");
    }

    return ret;
}

static mender_err_t
mender_troubleshoot_file_transfer_error_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    (void)response;
    mender_err_t ret = MENDER_OK;

    /* Treatment depending of the state machine */
    if (MENDER_TROUBLESHOOT_FILE_TRANSFER_IDLE != mender_troubleshoot_file_transfer_state_machine) {

        /* Close file */
        if (NULL != mender_troubleshoot_file_transfer_callbacks.close) {
            if (MENDER_OK != (ret = mender_troubleshoot_file_transfer_callbacks.close(mender_troubleshoot_file_transfer_handle))) {
                mender_log_error("Unable to close the file");
            }
        }
        mender_troubleshoot_file_transfer_state_machine = MENDER_TROUBLESHOOT_FILE_TRANSFER_IDLE;
    }

    return ret;
}

static mender_troubleshoot_file_transfer_get_file_t *
mender_troubleshoot_file_transfer_get_file_unpack(void *data, size_t length) {

    assert(NULL != data);
    msgpack_zone                                  zone;
    msgpack_object                                object;
    mender_troubleshoot_file_transfer_get_file_t *get_file = NULL;

    /* Unpack the message */
    if (MENDER_OK != mender_troubleshoot_msgpack_unpack_object(data, length, &zone, &object)) {
        mender_log_error("Unable to unpack the data");
        goto END;
    }

    /* Check object type */
    if ((MSGPACK_OBJECT_MAP != object.type) || (0 == object.via.map.size)) {
        mender_log_error("Invalid upload request object");
        goto END;
    }

    /* Decode get file */
    if (NULL == (get_file = mender_troubleshoot_file_transfer_get_file_decode(&object))) {
        mender_log_error("Invalid get file object");
        goto END;
    }

END:

    /* Release memory */
    msgpack_zone_destroy(&zone);

    return get_file;
}

static mender_troubleshoot_file_transfer_get_file_t *
mender_troubleshoot_file_transfer_get_file_decode(msgpack_object *object) {

    assert(NULL != object);
    mender_troubleshoot_file_transfer_get_file_t *get_file;

    /* Create get file */
    if (NULL == (get_file = (mender_troubleshoot_file_transfer_get_file_t *)malloc(sizeof(mender_troubleshoot_file_transfer_get_file_t)))) {
        mender_log_error("Unable to allocate memory");
        return NULL;
    }
    memset(get_file, 0, sizeof(mender_troubleshoot_file_transfer_get_file_t));

    /* Parse upload request */
    msgpack_object_kv *p = object->via.map.ptr;
    do {
        if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "path", p->key.via.str.size)) && (MSGPACK_OBJECT_STR == p->val.type)) {
            if (NULL == (get_file->path = (char *)malloc(p->val.via.str.size + 1))) {
                mender_log_error("Unable to allocate memory");
                goto FAIL;
            }
            memcpy(get_file->path, p->val.via.str.ptr, p->val.via.str.size);
            get_file->path[p->val.via.str.size] = '\0';
        }
        ++p;
    } while (p < object->via.map.ptr + object->via.map.size);

    return get_file;

FAIL:

    /* Release memory */
    mender_troubleshoot_file_transfer_get_file_release(get_file);

    return NULL;
}

static void
mender_troubleshoot_file_transfer_get_file_release(mender_troubleshoot_file_transfer_get_file_t *get_file) {

    /* Release memory */
    if (NULL != get_file) {
        if (NULL != get_file->path) {
            free(get_file->path);
        }
        free(get_file);
    }
}

static mender_troubleshoot_file_transfer_upload_request_t *
mender_troubleshoot_file_transfer_upload_request_unpack(void *data, size_t length) {

    assert(NULL != data);
    msgpack_zone                                        zone;
    msgpack_object                                      object;
    mender_troubleshoot_file_transfer_upload_request_t *upload_request = NULL;

    /* Unpack the message */
    if (MENDER_OK != mender_troubleshoot_msgpack_unpack_object(data, length, &zone, &object)) {
        mender_log_error("Unable to unpack the data");
        goto END;
    }

    /* Check object type */
    if ((MSGPACK_OBJECT_MAP != object.type) || (0 == object.via.map.size)) {
        mender_log_error("Invalid upload request object");
        goto END;
    }

    /* Decode upload request */
    if (NULL == (upload_request = mender_troubleshoot_file_transfer_upload_request_decode(&object))) {
        mender_log_error("Invalid upload request object");
        goto END;
    }

END:

    /* Release memory */
    msgpack_zone_destroy(&zone);

    return upload_request;
}

static mender_troubleshoot_file_transfer_upload_request_t *
mender_troubleshoot_file_transfer_upload_request_decode(msgpack_object *object) {

    assert(NULL != object);
    mender_troubleshoot_file_transfer_upload_request_t *upload_request;

    /* Create upload request */
    if (NULL == (upload_request = (mender_troubleshoot_file_transfer_upload_request_t *)malloc(sizeof(mender_troubleshoot_file_transfer_upload_request_t)))) {
        mender_log_error("Unable to allocate memory");
        return NULL;
    }
    memset(upload_request, 0, sizeof(mender_troubleshoot_file_transfer_upload_request_t));

    /* Parse upload request */
    msgpack_object_kv *p = object->via.map.ptr;
    do {
        if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "src_path", p->key.via.str.size)) && (MSGPACK_OBJECT_STR == p->val.type)) {
            if (NULL == (upload_request->src_path = (char *)malloc(p->val.via.str.size + 1))) {
                mender_log_error("Unable to allocate memory");
                goto FAIL;
            }
            memcpy(upload_request->src_path, p->val.via.str.ptr, p->val.via.str.size);
            upload_request->src_path[p->val.via.str.size] = '\0';
        } else if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "path", p->key.via.str.size)) && (MSGPACK_OBJECT_STR == p->val.type)) {
            if (NULL == (upload_request->path = (char *)malloc(p->val.via.str.size + 1))) {
                mender_log_error("Unable to allocate memory");
                goto FAIL;
            }
            memcpy(upload_request->path, p->val.via.str.ptr, p->val.via.str.size);
            upload_request->path[p->val.via.str.size] = '\0';
        }
        ++p;
    } while (p < object->via.map.ptr + object->via.map.size);

    return upload_request;

FAIL:

    /* Release memory */
    mender_troubleshoot_file_transfer_upload_request_release(upload_request);

    return NULL;
}

static void
mender_troubleshoot_file_transfer_upload_request_release(mender_troubleshoot_file_transfer_upload_request_t *upload_request) {

    /* Release memory */
    if (NULL != upload_request) {
        if (NULL != upload_request->src_path) {
            free(upload_request->src_path);
        }
        if (NULL != upload_request->path) {
            free(upload_request->path);
        }
        free(upload_request);
    }
}

static mender_troubleshoot_file_transfer_stat_file_t *
mender_troubleshoot_file_transfer_stat_file_unpack(void *data, size_t length) {

    assert(NULL != data);
    msgpack_zone                                   zone;
    msgpack_object                                 object;
    mender_troubleshoot_file_transfer_stat_file_t *stat_file = NULL;

    /* Unpack the message */
    if (MENDER_OK != mender_troubleshoot_msgpack_unpack_object(data, length, &zone, &object)) {
        mender_log_error("Unable to unpack the data");
        goto END;
    }

    /* Check object type */
    if ((MSGPACK_OBJECT_MAP != object.type) || (0 == object.via.map.size)) {
        mender_log_error("Invalid upload request object");
        goto END;
    }

    /* Decode stat file */
    if (NULL == (stat_file = mender_troubleshoot_file_transfer_stat_file_decode(&object))) {
        mender_log_error("Invalid stat file object");
        goto END;
    }

END:

    /* Release memory */
    msgpack_zone_destroy(&zone);

    return stat_file;
}

static mender_troubleshoot_file_transfer_stat_file_t *
mender_troubleshoot_file_transfer_stat_file_decode(msgpack_object *object) {

    assert(NULL != object);
    mender_troubleshoot_file_transfer_stat_file_t *stat_file;

    /* Create stat file */
    if (NULL == (stat_file = (mender_troubleshoot_file_transfer_stat_file_t *)malloc(sizeof(mender_troubleshoot_file_transfer_stat_file_t)))) {
        mender_log_error("Unable to allocate memory");
        return NULL;
    }
    memset(stat_file, 0, sizeof(mender_troubleshoot_file_transfer_stat_file_t));

    /* Parse stat file */
    msgpack_object_kv *p = object->via.map.ptr;
    do {
        if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "path", p->key.via.str.size)) && (MSGPACK_OBJECT_STR == p->val.type)) {
            if (NULL == (stat_file->path = (char *)malloc(p->val.via.str.size + 1))) {
                mender_log_error("Unable to allocate memory");
                goto FAIL;
            }
            memcpy(stat_file->path, p->val.via.str.ptr, p->val.via.str.size);
            stat_file->path[p->val.via.str.size] = '\0';
        }
        ++p;
    } while (p < object->via.map.ptr + object->via.map.size);

    return stat_file;

FAIL:

    /* Release memory */
    mender_troubleshoot_file_transfer_stat_file_release(stat_file);

    return NULL;
}

static void
mender_troubleshoot_file_transfer_stat_file_release(mender_troubleshoot_file_transfer_stat_file_t *stat_file) {

    /* Release memory */
    if (NULL != stat_file) {
        if (NULL != stat_file->path) {
            free(stat_file->path);
        }
        free(stat_file);
    }
}

static mender_err_t
mender_troubleshoot_file_transfer_file_info_pack(mender_troubleshoot_file_transfer_file_info_t *file_info, void **data, size_t *length) {

    assert(NULL != file_info);
    assert(NULL != data);
    assert(NULL != length);
    mender_err_t   ret = MENDER_OK;
    msgpack_object object;

    /* Encode file info */
    if (MENDER_OK != (ret = mender_troubleshoot_file_transfer_file_info_encode(file_info, &object))) {
        mender_log_error("Invalid file info object");
        goto FAIL;
    }

    /* Pack file info */
    if (MENDER_OK != (ret = mender_troubleshoot_msgpack_pack_object(&object, data, length))) {
        mender_log_error("Unable to pack file info object");
        goto FAIL;
    }

    /* Release memory */
    mender_troubleshoot_msgpack_release_object(&object);

    return ret;

FAIL:

    /* Release memory */
    mender_troubleshoot_msgpack_release_object(&object);

    return ret;
}

static mender_err_t
mender_troubleshoot_file_transfer_file_info_encode(mender_troubleshoot_file_transfer_file_info_t *file_info, msgpack_object *object) {

    assert(NULL != file_info);
    assert(NULL != object);
    mender_err_t       ret = MENDER_OK;
    msgpack_object_kv *p;

    /* Create file info */
    object->type = MSGPACK_OBJECT_MAP;
    if (0
        == (object->via.map.size = ((NULL != file_info->path) ? 1 : 0) + ((NULL != file_info->size) ? 1 : 0) + ((NULL != file_info->uid) ? 1 : 0)
                                   + ((NULL != file_info->gid) ? 1 : 0) + ((NULL != file_info->mode) ? 1 : 0) + ((NULL != file_info->time) ? 1 : 0))) {
        goto END;
    }
    if (NULL == (object->via.map.ptr = (msgpack_object_kv *)malloc(object->via.map.size * sizeof(struct msgpack_object_kv)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }

    /* Parse file info */
    p = object->via.map.ptr;
    if (NULL != file_info->path) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("path"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        p->key.via.str.size = (uint32_t)strlen("path");
        p->val.type         = MSGPACK_OBJECT_STR;
        p->val.via.str.size = (uint32_t)strlen(file_info->path);
        if (NULL == (p->val.via.str.ptr = (char *)malloc(p->val.via.str.size))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memcpy((void *)p->val.via.str.ptr, file_info->path, p->val.via.str.size);
        ++p;
    }
    if (NULL != file_info->size) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("size"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        p->key.via.str.size = (uint32_t)strlen("size");
        p->val.type         = MSGPACK_OBJECT_NEGATIVE_INTEGER;
        p->val.via.i64      = *file_info->size;
        ++p;
    }
    if (NULL != file_info->uid) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("uid"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        p->key.via.str.size = (uint32_t)strlen("uid");
        p->val.type         = MSGPACK_OBJECT_POSITIVE_INTEGER;
        p->val.via.u64      = *file_info->uid;
        ++p;
    }
    if (NULL != file_info->gid) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("gid"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        p->key.via.str.size = (uint32_t)strlen("gid");
        p->val.type         = MSGPACK_OBJECT_POSITIVE_INTEGER;
        p->val.via.u64      = *file_info->gid;
        ++p;
    }
    if (NULL != file_info->mode) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("mode"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        p->key.via.str.size = (uint32_t)strlen("mode");
        p->val.type         = MSGPACK_OBJECT_POSITIVE_INTEGER;
        p->val.via.u64      = *file_info->mode;
        ++p;
    }

    if (NULL != file_info->time) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("modtime"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        p->key.via.str.size = (uint32_t)strlen("modtime");
        p->val.type         = MSGPACK_OBJECT_EXT;
        p->val.via.ext.size = 4;
        p->val.via.ext.type = -1;
        if (NULL == (p->val.via.ext.ptr = malloc(p->val.via.ext.size))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memcpy((void *)p->val.via.ext.ptr, file_info->time, p->val.via.ext.size);
        ++p;
    }

END:
FAIL:

    return ret;
}

static void
mender_troubleshoot_file_transfer_file_info_release(mender_troubleshoot_file_transfer_file_info_t *file_info) {

    /* Release memory */
    if (NULL != file_info) {
        if (NULL != file_info->path) {
            free(file_info->path);
        }
        if (NULL != file_info->size) {
            free(file_info->size);
        }
        if (NULL != file_info->uid) {
            free(file_info->uid);
        }
        if (NULL != file_info->gid) {
            free(file_info->gid);
        }
        if (NULL != file_info->mode) {
            free(file_info->mode);
        }
        if (NULL != file_info->time) {
            free(file_info->time);
        }
        free(file_info);
    }
}

static mender_err_t
mender_troubleshoot_file_transfer_format_file_info(mender_troubleshoot_protomsg_t                *protomsg,
                                                   mender_troubleshoot_file_transfer_file_info_t *file_info,
                                                   mender_troubleshoot_protomsg_t               **response) {

    assert(NULL != protomsg);
    assert(NULL != protomsg->hdr);
    mender_err_t ret = MENDER_OK;

    /* Format file transfer file info message */
    if (NULL == (*response = (mender_troubleshoot_protomsg_t *)malloc(sizeof(mender_troubleshoot_protomsg_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(*response, 0, sizeof(mender_troubleshoot_protomsg_t));
    if (NULL == ((*response)->hdr = (mender_troubleshoot_protomsg_hdr_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset((*response)->hdr, 0, sizeof(mender_troubleshoot_protomsg_hdr_t));
    (*response)->hdr->proto = protomsg->hdr->proto;
    if (NULL == ((*response)->hdr->typ = strdup(MENDER_TROUBLESHOOT_FILE_TRANSFER_MESSAGE_TYPE_FILE_INFO))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL != protomsg->hdr->sid) {
        if (NULL == ((*response)->hdr->sid = strdup(protomsg->hdr->sid))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
    }
    if (NULL != protomsg->hdr->properties) {
        if (NULL
            == ((*response)->hdr->properties
                = (mender_troubleshoot_protomsg_hdr_properties_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_properties_t)))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memset((*response)->hdr->properties, 0, sizeof(mender_troubleshoot_protomsg_hdr_properties_t));
        if (NULL != protomsg->hdr->properties->user_id) {
            if (NULL == ((*response)->hdr->properties->user_id = strdup(protomsg->hdr->properties->user_id))) {
                mender_log_error("Unable to allocate memory");
                ret = MENDER_FAIL;
                goto FAIL;
            }
        }
    }
    if (NULL == ((*response)->body = (mender_troubleshoot_protomsg_body_t *)malloc(sizeof(mender_troubleshoot_protomsg_body_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset((*response)->body, 0, sizeof(mender_troubleshoot_protomsg_body_t));

    /* Encode and pack file info data message */
    if (MENDER_OK != (ret = mender_troubleshoot_file_transfer_file_info_pack(file_info, &((*response)->body->data), &((*response)->body->length)))) {
        mender_log_error("Unable to encode message");
        goto FAIL;
    }

    return ret;

FAIL:

    /* Release memory */
    mender_troubleshoot_protomsg_release(*response);
    *response = NULL;

    return ret;
}

static mender_err_t
mender_troubleshoot_file_transfer_format_ack(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    assert(NULL != protomsg->hdr);
    mender_err_t ret = MENDER_OK;

    /* Format file transfer ack message */
    if (NULL == (*response = (mender_troubleshoot_protomsg_t *)malloc(sizeof(mender_troubleshoot_protomsg_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(*response, 0, sizeof(mender_troubleshoot_protomsg_t));
    if (NULL == ((*response)->hdr = (mender_troubleshoot_protomsg_hdr_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset((*response)->hdr, 0, sizeof(mender_troubleshoot_protomsg_hdr_t));
    (*response)->hdr->proto = protomsg->hdr->proto;
    if (NULL == ((*response)->hdr->typ = strdup(MENDER_TROUBLESHOOT_FILE_TRANSFER_MESSAGE_TYPE_ACK))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL != protomsg->hdr->sid) {
        if (NULL == ((*response)->hdr->sid = strdup(protomsg->hdr->sid))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
    }
    if (NULL != protomsg->hdr->properties) {
        if (NULL
            == ((*response)->hdr->properties
                = (mender_troubleshoot_protomsg_hdr_properties_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_properties_t)))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memset((*response)->hdr->properties, 0, sizeof(mender_troubleshoot_protomsg_hdr_properties_t));
        if (NULL != protomsg->hdr->properties->user_id) {
            if (NULL == ((*response)->hdr->properties->user_id = strdup(protomsg->hdr->properties->user_id))) {
                mender_log_error("Unable to allocate memory");
                ret = MENDER_FAIL;
                goto FAIL;
            }
        }

        if (NULL == ((*response)->hdr->properties->offset = (size_t *)malloc(sizeof(size_t)))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        if (NULL != protomsg->hdr->properties->offset) {
            *((*response)->hdr->properties->offset) = *(protomsg->hdr->properties->offset);
        } else {
            *((*response)->hdr->properties->offset) = 0;
        }
    }

    return ret;

FAIL:

    /* Release memory */
    mender_troubleshoot_protomsg_release(*response);
    *response = NULL;

    return ret;
}

static mender_err_t
mender_troubleshoot_file_transfer_format_error(mender_troubleshoot_protomsg_t            *protomsg,
                                               mender_troubleshoot_file_transfer_error_t *error,
                                               mender_troubleshoot_protomsg_t           **response) {

    assert(NULL != protomsg);
    assert(NULL != protomsg->hdr);
    mender_err_t ret = MENDER_OK;

    /* Format file transfer error message */
    if (NULL == (*response = (mender_troubleshoot_protomsg_t *)malloc(sizeof(mender_troubleshoot_protomsg_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(*response, 0, sizeof(mender_troubleshoot_protomsg_t));
    if (NULL == ((*response)->hdr = (mender_troubleshoot_protomsg_hdr_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset((*response)->hdr, 0, sizeof(mender_troubleshoot_protomsg_hdr_t));
    (*response)->hdr->proto = protomsg->hdr->proto;
    if (NULL == ((*response)->hdr->typ = strdup(MENDER_TROUBLESHOOT_FILE_TRANSFER_MESSAGE_TYPE_ERROR))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL != protomsg->hdr->sid) {
        if (NULL == ((*response)->hdr->sid = strdup(protomsg->hdr->sid))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
    }
    if (NULL != protomsg->hdr->properties) {
        if (NULL
            == ((*response)->hdr->properties
                = (mender_troubleshoot_protomsg_hdr_properties_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_properties_t)))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memset((*response)->hdr->properties, 0, sizeof(mender_troubleshoot_protomsg_hdr_properties_t));
        if (NULL != protomsg->hdr->properties->user_id) {
            if (NULL == ((*response)->hdr->properties->user_id = strdup(protomsg->hdr->properties->user_id))) {
                mender_log_error("Unable to allocate memory");
                ret = MENDER_FAIL;
                goto FAIL;
            }
        }
    }
    if (NULL == ((*response)->body = (mender_troubleshoot_protomsg_body_t *)malloc(sizeof(mender_troubleshoot_protomsg_body_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset((*response)->body, 0, sizeof(mender_troubleshoot_protomsg_body_t));

    /* Encode and pack error message */
    if (MENDER_OK != (ret = mender_troubleshoot_file_transfer_error_message_pack(error, &((*response)->body->data), &((*response)->body->length)))) {
        mender_log_error("Unable to encode message");
        goto FAIL;
    }

    return ret;

FAIL:

    /* Release memory */
    mender_troubleshoot_protomsg_release(*response);
    *response = NULL;

    return ret;
}

/**
 * @brief Encode and pack error message
 * @param error Error
 * @param data Packed data encoded
 * @param length Length of the data encoded
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t
mender_troubleshoot_file_transfer_error_message_pack(mender_troubleshoot_file_transfer_error_t *error, void **data, size_t *length) {

    assert(NULL != error);
    assert(NULL != data);
    assert(NULL != length);
    mender_err_t   ret = MENDER_OK;
    msgpack_object object;

    /* Encode error message */
    if (MENDER_OK != (ret = mender_troubleshoot_file_transfer_error_message_encode(error, &object))) {
        mender_log_error("Invalid error message object");
        goto FAIL;
    }

    /* Pack error message */
    if (MENDER_OK != (ret = mender_troubleshoot_msgpack_pack_object(&object, data, length))) {
        mender_log_error("Unable to pack error message object");
        goto FAIL;
    }

    /* Release memory */
    mender_troubleshoot_msgpack_release_object(&object);

    return ret;

FAIL:

    /* Release memory */
    mender_troubleshoot_msgpack_release_object(&object);

    return ret;
}

static mender_err_t
mender_troubleshoot_file_transfer_error_message_encode(mender_troubleshoot_file_transfer_error_t *error, msgpack_object *object) {

    assert(NULL != error);
    assert(NULL != object);
    mender_err_t       ret = MENDER_OK;
    msgpack_object_kv *p;

    /* Create error */
    object->type = MSGPACK_OBJECT_MAP;
    if (0 == (object->via.map.size = ((NULL != error->description) ? 1 : 0) + ((NULL != error->type) ? 1 : 0) + ((NULL != error->id) ? 1 : 0))) {
        goto END;
    }
    if (NULL == (object->via.map.ptr = (msgpack_object_kv *)malloc(object->via.map.size * sizeof(struct msgpack_object_kv)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }

    /* Parse error */
    p = object->via.map.ptr;
    if (NULL != error->description) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("err"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        p->key.via.str.size = (uint32_t)strlen("err");
        p->val.type         = MSGPACK_OBJECT_STR;
        p->val.via.str.size = (uint32_t)strlen(error->description);
        if (NULL == (p->val.via.str.ptr = (char *)malloc(p->val.via.str.size))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memcpy((void *)p->val.via.str.ptr, error->description, p->val.via.str.size);
        ++p;
    }
    if (NULL != error->type) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("msgtype"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        p->key.via.str.size = (uint32_t)strlen("msgtype");
        p->val.type         = MSGPACK_OBJECT_STR;
        p->val.via.str.size = (uint32_t)strlen(error->type);
        if (NULL == (p->val.via.str.ptr = (char *)malloc(p->val.via.str.size))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memcpy((void *)p->val.via.str.ptr, error->type, p->val.via.str.size);
        ++p;
    }
    if (NULL != error->id) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("msgid"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        p->key.via.str.size = (uint32_t)strlen("msgid");
        p->val.type         = MSGPACK_OBJECT_STR;
        p->val.via.str.size = (uint32_t)strlen(error->id);
        if (NULL == (p->val.via.str.ptr = (char *)malloc(p->val.via.str.size))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memcpy((void *)p->val.via.str.ptr, error->id, p->val.via.str.size);
        ++p;
    }

END:
FAIL:

    return ret;
}

static mender_err_t
mender_troubleshoot_file_transfer_send_chunk(char *sid, char *user_id, size_t offset, void *data, size_t length) {

    assert(NULL != sid);
    assert(NULL != user_id);
    mender_troubleshoot_protomsg_t *protomsg = NULL;
    mender_err_t                    ret      = MENDER_OK;
    void                           *payload  = NULL;

    /* Send file transfer chunk message */
    if (NULL == (protomsg = (mender_troubleshoot_protomsg_t *)malloc(sizeof(mender_troubleshoot_protomsg_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(protomsg, 0, sizeof(mender_troubleshoot_protomsg_t));
    if (NULL == (protomsg->hdr = (mender_troubleshoot_protomsg_hdr_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(protomsg->hdr, 0, sizeof(mender_troubleshoot_protomsg_hdr_t));
    protomsg->hdr->proto = MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROTO_FILE_TRANSFER;
    if (NULL == (protomsg->hdr->typ = strdup(MENDER_TROUBLESHOOT_FILE_TRANSFER_MESSAGE_TYPE_CHUNK))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == (protomsg->hdr->sid = strdup(sid))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == (protomsg->hdr->properties = (mender_troubleshoot_protomsg_hdr_properties_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_properties_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(protomsg->hdr->properties, 0, sizeof(mender_troubleshoot_protomsg_hdr_properties_t));
    if (NULL == (protomsg->hdr->properties->user_id = strdup(user_id))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == (protomsg->hdr->properties->offset = (size_t *)malloc(sizeof(size_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    *protomsg->hdr->properties->offset = offset;
    if ((NULL != data) && (0 != length)) {
        if (NULL == (protomsg->body = (mender_troubleshoot_protomsg_body_t *)malloc(sizeof(mender_troubleshoot_protomsg_body_t)))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memset(protomsg->body, 0, sizeof(mender_troubleshoot_protomsg_body_t));
        if (NULL == (protomsg->body->data = malloc(length))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memcpy(protomsg->body->data, data, length);
        protomsg->body->length = length;
    }

    /* Encode and pack the message */
    if (MENDER_OK != (ret = mender_troubleshoot_protomsg_pack(protomsg, &payload, &length))) {
        mender_log_error("Unable to encode message");
        goto FAIL;
    }

    /* Send message */
    if (MENDER_OK != (ret = mender_troubleshoot_api_send(payload, length))) {
        mender_log_error("Unable to send message");
        goto FAIL;
    }

FAIL:

    /* Release memory */
    mender_troubleshoot_protomsg_release(protomsg);
    if (NULL != payload) {
        free(payload);
    }

    return ret;
}

#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_FILE_TRANSFER */
#endif /* CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT */
