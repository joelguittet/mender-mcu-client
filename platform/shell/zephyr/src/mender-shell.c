/**
 * @file      mender-shell.c
 * @brief     Mender shell interface for Zephyr platform
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

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/ring_buffer.h>
#include "mender-log.h"
#include "mender-troubleshoot.h"

/**
 * @brief Default prompt
 */
#ifndef CONFIG_MENDER_SHELL_PROMPT
#define CONFIG_MENDER_SHELL_PROMPT "~$ "
#endif /* CONFIG_MENDER_SHELL_PROMPT */

/**
 * @brief Default rx ring buffer size
 */
#ifndef CONFIG_MENDER_SHELL_RX_RING_BUFFER_SIZE
#define CONFIG_MENDER_SHELL_RX_RING_BUFFER_SIZE (64)
#endif /* CONFIG_MENDER_SHELL_RX_RING_BUFFER_SIZE */

/**
 * @brief Default tx ring buffer size
 */
#ifndef CONFIG_MENDER_SHELL_TX_RING_BUFFER_SIZE
#define CONFIG_MENDER_SHELL_TX_RING_BUFFER_SIZE (512)
#endif /* CONFIG_MENDER_SHELL_TX_RING_BUFFER_SIZE */

/**
 * @brief Default tx work queue stack size
 */
#ifndef CONFIG_MENDER_SHELL_TX_WORK_QUEUE_STACK_SIZE
#define CONFIG_MENDER_SHELL_TX_WORK_QUEUE_STACK_SIZE (2048)
#endif /* CONFIG_MENDER_SHELL_TX_WORK_QUEUE_STACK_SIZE */

/**
 * @brief Default tx work queue priority
 */
#ifndef CONFIG_MENDER_SHELL_TX_WORK_QUEUE_PRIORITY
#define CONFIG_MENDER_SHELL_TX_WORK_QUEUE_PRIORITY (5)
#endif /* CONFIG_MENDER_SHELL_TX_WORK_QUEUE_PRIORITY */

/**
 * @brief Default tx work delay (milliseconds)
 */
#ifndef CONFIG_MENDER_SHELL_TX_WORK_DELAY
#define CONFIG_MENDER_SHELL_TX_WORK_DELAY (100)
#endif /* CONFIG_MENDER_SHELL_TX_WORK_DELAY */

/**
 * @brief Default log backend level (no logs)
 */
#ifndef CONFIG_MENDER_SHELL_LOG_BACKEND_LEVEL
#define CONFIG_MENDER_SHELL_LOG_BACKEND_LEVEL (LOG_LEVEL_NONE)
#endif /* CONFIG_MENDER_SHELL_LOG_BACKEND_LEVEL */

/**
 * @brief Default log backend message queue size (bytes)
 */
#ifndef CONFIG_MENDER_SHELL_LOG_BACKEND_MESSAGE_QUEUE_SIZE
#define CONFIG_MENDER_SHELL_LOG_BACKEND_MESSAGE_QUEUE_SIZE (512)
#endif /* CONFIG_MENDER_SHELL_LOG_BACKEND_MESSAGE_QUEUE_SIZE */

/**
 * @brief Default log backend message queue timeout (milliseconds)
 */
#ifndef CONFIG_MENDER_SHELL_LOG_BACKEND_MESSAGE_QUEUE_TIMEOUT
#define CONFIG_MENDER_SHELL_LOG_BACKEND_MESSAGE_QUEUE_TIMEOUT (100)
#endif /* CONFIG_MENDER_SHELL_LOG_BACKEND_MESSAGE_QUEUE_TIMEOUT */

/**
 * @brief Mender RTOS work queue stack
 */
K_THREAD_STACK_DEFINE(mender_shell_tx_work_queue_stack, CONFIG_MENDER_SHELL_TX_WORK_QUEUE_STACK_SIZE);

/**
 * @brief Mender shell context
 */
typedef struct {
    shell_transport_handler_t evt_handler;                                        /**< Handler function registered by shell init */
    void                     *context;                                            /**< Context registered by shell init */
    struct ring_buf           rx_ringbuf;                                         /**< Rx ring buffer handler */
    uint8_t                   rx_buffer[CONFIG_MENDER_SHELL_RX_RING_BUFFER_SIZE]; /**< Rx ring buffer */
    struct ring_buf           tx_ringbuf;                                         /**< Tx ring buffer handler */
    uint8_t                   tx_buffer[CONFIG_MENDER_SHELL_TX_RING_BUFFER_SIZE]; /**< Tx ring buffer */
    struct k_work_q           tx_work_queue_handle;                               /**< Tx work queue handle */
    struct k_work_delayable   tx_work_handle;                                     /**< Tx work handle */
} mender_shell_context_t;

/**
 * @brief Mender shell context
 */
static mender_shell_context_t mender_shell_context;

static void
mender_shell_tx_work_handler(struct k_work *work) {

    (void)work;
    uint8_t *buffer;
    size_t   length;

    /* Retrieve length of data available in the tx ring buffer */
    if ((length = ring_buf_size_get(&mender_shell_context.tx_ringbuf)) > 0) {

        /* Send data to the shell on the mender server */
        if (NULL == (buffer = malloc(length))) {
            mender_log_error("Unable to allocate memory");
        } else {
            ring_buf_get(&mender_shell_context.tx_ringbuf, buffer, (uint32_t)length);
            mender_troubleshoot_shell_print(buffer, length);
            free(buffer);
        }
    }
}

static int
mender_shell_transport_api_init(const struct shell_transport *transport, const void *config, shell_transport_handler_t evt_handler, void *context) {

    assert(NULL != transport);
    (void)config;
    assert(NULL != evt_handler);
    mender_shell_context_t *ctx = (mender_shell_context_t *)transport->ctx;

    /* Initialize shell context */
    memset(ctx, 0, sizeof(mender_shell_context_t));
    ctx->evt_handler = evt_handler;
    ctx->context     = context;

    /* Initialize shell tx work queue */
    k_work_queue_init(&ctx->tx_work_queue_handle);
    k_work_queue_start(&ctx->tx_work_queue_handle,
                       mender_shell_tx_work_queue_stack,
                       CONFIG_MENDER_SHELL_TX_WORK_QUEUE_STACK_SIZE,
                       CONFIG_MENDER_SHELL_TX_WORK_QUEUE_PRIORITY,
                       NULL);
    k_thread_name_set(k_work_queue_thread_get(&ctx->tx_work_queue_handle), "mender_shell_tx_work_queue");

    /* Initialize ring buffers */
    ring_buf_init(&ctx->rx_ringbuf, CONFIG_MENDER_SHELL_RX_RING_BUFFER_SIZE, ctx->rx_buffer);
    ring_buf_init(&ctx->tx_ringbuf, CONFIG_MENDER_SHELL_TX_RING_BUFFER_SIZE, ctx->tx_buffer);

    /* Initialize tx work handle */
    k_work_init_delayable(&ctx->tx_work_handle, mender_shell_tx_work_handler);

    return 0;
}

static int
mender_shell_transport_api_uninit(const struct shell_transport *transport) {

    assert(NULL != transport);
    mender_shell_context_t *ctx = (mender_shell_context_t *)transport->ctx;

    /* Cancel pending tx work */
    k_work_cancel_delayable(&ctx->tx_work_handle);

    /* Reset ring buffers */
    ring_buf_reset(&ctx->rx_ringbuf);
    ring_buf_reset(&ctx->tx_ringbuf);

    return 0;
}

static int
mender_shell_transport_api_enable(const struct shell_transport *transport, bool blocking) {

    (void)transport;
    (void)blocking;

    /* Not supported */

    return 0;
}

static int
mender_shell_transport_api_write(const struct shell_transport *transport, const void *data, size_t length, size_t *cnt) {

    assert(NULL != transport);
    assert(NULL != data);
    assert(NULL != cnt);
    mender_shell_context_t *ctx = (mender_shell_context_t *)transport->ctx;
    uint8_t                *buffer;

    /* Cancel pending tx work */
    k_work_cancel_delayable(&ctx->tx_work_handle);

    /* Add data to the tx ring buffer */
    if (length != ring_buf_put(&ctx->tx_ringbuf, data, (uint32_t)length)) {
        mender_log_error("Unable to write data to the shell");
        *cnt = 0;
        goto END;
    }
    *cnt = length;

    /* Retrieve length of data available in the tx ring buffer */
    if ((length = ring_buf_size_get(&mender_shell_context.tx_ringbuf)) > CONFIG_MENDER_SHELL_TX_RING_BUFFER_SIZE / 4) {

        /* Send data to the shell on the mender server */
        if (NULL == (buffer = malloc(length))) {
            mender_log_error("Unable to allocate memory");
        } else {
            ring_buf_get(&ctx->tx_ringbuf, buffer, (uint32_t)length);
            mender_troubleshoot_shell_print(buffer, length);
            free(buffer);
        }
    }

    /* If tx ring buffer is not empty, schedule delayed tx work to flush the ring buffer */
    if (!ring_buf_is_empty(&ctx->tx_ringbuf)) {
        k_work_reschedule_for_queue(&ctx->tx_work_queue_handle, &ctx->tx_work_handle, K_MSEC(CONFIG_MENDER_SHELL_TX_WORK_DELAY));
    }

    /* Invoke event handler to signal data have been written */
    ctx->evt_handler(SHELL_TRANSPORT_EVT_TX_RDY, ctx->context);

END:

    return 0;
}

static int
mender_shell_transport_api_read(const struct shell_transport *transport, void *data, size_t length, size_t *cnt) {

    assert(NULL != transport);
    assert(NULL != data);
    assert(NULL != cnt);
    mender_shell_context_t *ctx = (mender_shell_context_t *)transport->ctx;

    /* Read data from the ring buffer */
    *cnt = ring_buf_get(&ctx->rx_ringbuf, data, (uint32_t)length);

    return 0;
}

/**
 * @brief Mender shell transport API
 */
const struct shell_transport_api mender_shell_transport_api = { .init   = mender_shell_transport_api_init,
                                                                .uninit = mender_shell_transport_api_uninit,
                                                                .enable = mender_shell_transport_api_enable,
                                                                .write  = mender_shell_transport_api_write,
                                                                .read   = mender_shell_transport_api_read };

/**
 * @brief Mender shell transport
 */
struct shell_transport mender_shell_transport = { .api = &mender_shell_transport_api, .ctx = &mender_shell_context };

/**
 * @brief Mender shell definition
 */
SHELL_DEFINE(mender_shell,
             CONFIG_MENDER_SHELL_PROMPT,
             &mender_shell_transport,
             CONFIG_MENDER_SHELL_LOG_BACKEND_MESSAGE_QUEUE_SIZE,
             CONFIG_MENDER_SHELL_LOG_BACKEND_MESSAGE_QUEUE_TIMEOUT,
             SHELL_FLAG_OLF_CRLF);

static int
mender_shell_init(void) {

    /* Initialization of the shell */
    bool     log_backend = CONFIG_MENDER_SHELL_LOG_BACKEND_LEVEL > LOG_LEVEL_NONE;
    uint32_t level       = (CONFIG_MENDER_SHELL_LOG_BACKEND_LEVEL > LOG_LEVEL_DBG) ? CONFIG_LOG_MAX_LEVEL : CONFIG_MENDER_SHELL_LOG_BACKEND_LEVEL;
    static const struct shell_backend_config_flags cfg_flags = SHELL_DEFAULT_BACKEND_CONFIG_FLAGS;
    return shell_init(&mender_shell, NULL, cfg_flags, log_backend, level);
}

/**
 * @brief Mender shell initialization
 */
SYS_INIT(mender_shell_init, POST_KERNEL, 0);

mender_err_t
mender_shell_open(uint16_t terminal_width, uint16_t terminal_height) {

    (void)terminal_height;
    (void)terminal_width;

    /* Start shell */
    if (!IS_ENABLED(CONFIG_SHELL_AUTOSTART)) {
        shell_start(&mender_shell);
    }

    return MENDER_OK;
}

mender_err_t
mender_shell_resize(uint16_t terminal_width, uint16_t terminal_height) {

    (void)terminal_height;
    (void)terminal_width;

    /* Not supported */

    return MENDER_OK;
}

mender_err_t
mender_shell_write(void *data, size_t length) {

    mender_err_t ret = MENDER_OK;

    /* Check if event handler is defined */
    if (NULL == mender_shell_context.evt_handler) {
        mender_log_error("Unable to write data to the shell");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Copy data to the rx ring buffer */
    if (length != ring_buf_put(&mender_shell_context.rx_ringbuf, data, (uint32_t)length)) {
        mender_log_error("Unable to write data to the shell");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Invoke event handler to signal data are available */
    mender_shell_context.evt_handler(SHELL_TRANSPORT_EVT_RX_RDY, mender_shell_context.context);

END:

    return ret;
}

mender_err_t
mender_shell_close(void) {

    /* Stop shell */
    if (!IS_ENABLED(CONFIG_SHELL_AUTOSTART)) {
        shell_stop(&mender_shell);
    }

    return MENDER_OK;
}
