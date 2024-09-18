/**
 * @file      mender-troubleshoot-mender-client.c
 * @brief     Mender MCU Troubleshoot add-on mender-client message handler implementation
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

#include "mender-client.h"
#include "mender-inventory.h"
#include "mender-log.h"
#include "mender-troubleshoot-mender-client.h"

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT

/**
 * @brief Message type
 */
#define MENDER_TROUBLESHOOT_MENDER_CLIENT_MESSAGE_TYPE_CHECK_UPDATE   "check-update"
#define MENDER_TROUBLESHOOT_MENDER_CLIENT_MESSAGE_TYPE_SEND_INVENTORY "send-inventory"

/**
 * @brief Function called to perform the treatment of the mender client check update messages
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_mender_client_check_update_message_handler(mender_troubleshoot_protomsg_t  *protomsg,
                                                                                   mender_troubleshoot_protomsg_t **response);

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY

/**
 * @brief Function called to perform the treatment of the mender client send inventory messages
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_mender_client_send_inventory_message_handler(mender_troubleshoot_protomsg_t  *protomsg,
                                                                                     mender_troubleshoot_protomsg_t **response);

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY */

/**
 * @brief Function used to format acknowledgment messages
 * @param protomsg Received proto message
 * @param status Status
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_mender_client_format_ack(mender_troubleshoot_protomsg_t                      *protomsg,
                                                                 mender_troubleshoot_protomsg_hdr_properties_status_t status,
                                                                 mender_troubleshoot_protomsg_t                     **response);

mender_err_t
mender_troubleshoot_mender_client_init(void) {

    /* Nothing to do */
    return MENDER_OK;
}

mender_err_t
mender_troubleshoot_mender_client_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

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
    if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_MENDER_CLIENT_MESSAGE_TYPE_CHECK_UPDATE)) {
        /* Check update */
        ret = mender_troubleshoot_mender_client_check_update_message_handler(protomsg, response);
#ifdef CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY
    } else if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_MENDER_CLIENT_MESSAGE_TYPE_SEND_INVENTORY)) {
        /* Send inventory */
        ret = mender_troubleshoot_mender_client_send_inventory_message_handler(protomsg, response);
#endif /* CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY */
    } else {
        /* Not supported */
        mender_log_error("Unsupported mender client message received with message type '%s'", protomsg->hdr->typ);
        ret = MENDER_FAIL;
        goto FAIL;
    }

FAIL:

    return ret;
}

mender_err_t
mender_troubleshoot_mender_client_exit(void) {

    /* Nothing to do */
    return MENDER_OK;
}

static mender_err_t
mender_troubleshoot_mender_client_check_update_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    mender_err_t ret = MENDER_OK;

    /* Trigger execution of the mender-client update work */
    if (MENDER_OK != (ret = mender_client_execute())) {
        mender_log_error("Unable to execute mender-client");
        goto FAIL;
    }

    /* Format acknowledgment */
    if (MENDER_OK
        != (ret = mender_troubleshoot_mender_client_format_ack(protomsg,
                                                               (MENDER_OK == ret) ? MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROPERTIES_STATUS_TYPE_NORMAL
                                                                                  : MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROPERTIES_STATUS_TYPE_ERROR,
                                                               response))) {
        mender_log_error("Unable to format acknowledgment");
        goto FAIL;
    }

FAIL:

    return ret;
}

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY

static mender_err_t
mender_troubleshoot_mender_client_send_inventory_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    mender_err_t ret = MENDER_OK;

    /* Trigger execution of the mender-inventory work */
    if (MENDER_OK != (ret = mender_inventory_execute())) {
        mender_log_error("Unable to execute mender-inventory");
        goto FAIL;
    }

    /* Format acknowledgment */
    if (MENDER_OK
        != (ret = mender_troubleshoot_mender_client_format_ack(protomsg,
                                                               (MENDER_OK == ret) ? MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROPERTIES_STATUS_TYPE_NORMAL
                                                                                  : MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROPERTIES_STATUS_TYPE_ERROR,
                                                               response))) {
        mender_log_error("Unable to format acknowledgment");
        goto FAIL;
    }

FAIL:

    return ret;
}

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY */

static mender_err_t
mender_troubleshoot_mender_client_format_ack(mender_troubleshoot_protomsg_t                      *protomsg,
                                             mender_troubleshoot_protomsg_hdr_properties_status_t status,
                                             mender_troubleshoot_protomsg_t                     **response) {

    assert(NULL != protomsg);
    assert(NULL != protomsg->hdr);
    mender_err_t ret = MENDER_OK;

    /* Format mender client ack message */
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
    if (NULL == ((*response)->hdr->typ = strdup(protomsg->hdr->typ))) {
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
    if (NULL
        == ((*response)->hdr->properties = (mender_troubleshoot_protomsg_hdr_properties_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_properties_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset((*response)->hdr->properties, 0, sizeof(mender_troubleshoot_protomsg_hdr_properties_t));
    if (NULL
        == ((*response)->hdr->properties->status
            = (mender_troubleshoot_protomsg_hdr_properties_status_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_properties_status_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    *((*response)->hdr->properties->status) = status;

    return ret;

FAIL:

    /* Release memory */
    mender_troubleshoot_protomsg_release(*response);
    *response = NULL;

    return ret;
}

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT */
