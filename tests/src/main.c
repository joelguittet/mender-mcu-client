/**
 * @file      main.c
 * @brief     Test application used to perform SonarCloud analysis
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

#include <getopt.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include "mender-client.h"
#include "mender-configure.h"
#include "mender-inventory.h"
#include "mender-log.h"
#include "mender-ota.h"

/**
 * @brief Mender client options
 */
static const struct option mender_client_options[] = { { "help", 0, NULL, 'h' },        { "mac_address", 1, NULL, 'm' },  { "artifact_name", 1, NULL, 'a' },
                                                       { "device_type", 1, NULL, 'd' }, { "tenant_token", 1, NULL, 't' }, { NULL, 0, NULL, 0 } };

/**
 * @brief Mender client events
 */
static pthread_mutex_t mender_client_events_mutex;
static pthread_cond_t  mender_client_events_cond;

/**
 * @brief Authentication success callback
 * @return MENDER_OK if application is marked valid and success deployment status should be reported to the server, error code otherwise
 */
static mender_err_t
authentication_success_cb(void) {

    mender_err_t ret;

    mender_log_info("Mender client authenticated");

    /* Activate mender add-ons */
    /* The application can activate each add-on depending of the current status of the device */
#ifdef CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE
    if (MENDER_OK != (ret = mender_configure_activate())) {
        mender_log_error("Unable to activate configure add-on");
        return ret;
    }
#endif /* CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE */
#ifdef CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY
    if (MENDER_OK != (ret = mender_inventory_activate())) {
        mender_log_error("Unable to activate inventory add-on");
        return ret;
    }
#endif /* CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY */

    /* Validate the image if it is still pending */
    /* Note it is possible to do multiple diagnosic tests before validating the image */
    if (MENDER_OK != (ret = mender_ota_mark_app_valid_cancel_rollback())) {
        mender_log_error("Unable to validate the image");
        return ret;
    }

    return ret;
}

/**
 * @brief Authentication failure callback
 * @return MENDER_OK if nothing to do, error code if the mender client should restart the application
 */
static mender_err_t
authentication_failure_cb(void) {

    mender_err_t ret;

    /* Increment number of failures */
    mender_log_info("Mender client authentication failed");

    /* Invalidate the image if it is still pending */
    if (MENDER_OK != (ret = mender_ota_mark_app_invalid_rollback_and_reboot())) {
        mender_log_error("Unable to invalidate the image");
        return ret;
    }

    return ret;
}

/**
 * @brief Deployment status callback
 * @param status Deployment status value
 * @param desc Deployment status description as string
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t
deployment_status_cb(mender_deployment_status_t status, char *desc) {

    /* We can do something else if required */
    mender_log_info("Deployment status is '%s'", desc);

    return MENDER_OK;
}

/**
 * @brief Restart callback
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t
restart_cb(void) {

    /* Application is responsible to shutdown and restart the system now */
    pthread_mutex_lock(&mender_client_events_mutex);
    pthread_cond_signal(&mender_client_events_cond);
    pthread_mutex_unlock(&mender_client_events_mutex);

    return MENDER_OK;
}

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE
#ifndef CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE

/**
 * @brief Device configuration updated
 * @param configuration Device configuration
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t
config_updated_cb(mender_keystore_t *configuration) {

    /* Application can use the new device configuration now */
    if (NULL != configuration) {
        size_t index = 0;
        mender_log_info("Device configuration received from the server");
        while ((NULL != configuration[index].name) && (NULL != configuration[index].value)) {
            mender_log_info("Key=%s, value=%s", configuration[index].name, configuration[index].value);
            index++;
        }
    }

    return MENDER_OK;
}

#endif /* CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE */
#endif /* CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE */

/**
 * @brief Signal handler
 * @param signo Signal number
 * @param info Signal information
 * @param arg Unused
 */
static void
sig_handler(int signo, siginfo_t *info, void *arg) {

    assert(NULL != info);
    (void)arg;

    /* Signal handling */
    mender_log_info("Signal '%d' received from process '%d'", signo, info->si_pid);
    if ((SIGINT == signo) || (SIGTERM == signo)) {
        pthread_mutex_lock(&mender_client_events_mutex);
        pthread_cond_signal(&mender_client_events_cond);
        pthread_mutex_unlock(&mender_client_events_mutex);
    }
}

/**
 * @brief Main function
 * @param argc Number of arguments
 * @param argv Arguments
 * @return EXIT_SUCCESS if the program succeeds, EXIT_FAILURE otherwise
 */
int
main(int argc, char **argv) {

    int   ret           = EXIT_SUCCESS;
    char *mac_address   = NULL;
    char *artifact_name = NULL;
    char *device_type   = NULL;
    char *tenant_token  = NULL;

    /* Initialize sig handler */
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_sigaction = sig_handler;
    action.sa_flags     = SA_SIGINFO;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);

    /* Parse options */
    int opt;
    while (-1 != (opt = getopt_long(argc, argv, "hm:a:d:t:", mender_client_options, NULL))) {
        switch (opt) {
            case 'h':
                /* Help */
                printf("usage: %s [options]\n", (strrchr(argv[0], '/') ? strrchr(argv[0], '/') + 1 : argv[0]));
                printf("\t--help, -h: Print this help\n");
                printf("\t--mac_address, -m: MAC address\n");
                printf("\t--artifact_name, -a: Artifact name\n");
                printf("\t--device_type, -d: Device type\n");
                printf("\t--tenant_token, -t: Tenant token (optional)\n");
                goto END;
                break;
            case 'm':
                /* MAC address */
                mac_address = strdup(optarg);
                break;
            case 'a':
                /* Artifact name */
                artifact_name = strdup(optarg);
                break;
            case 'd':
                /* Device type */
                device_type = strdup(optarg);
                break;
            case 't':
                /* Tenant token */
                tenant_token = strdup(optarg);
                break;
            default:
                /* Unknown option */
                ret = EXIT_FAILURE;
                goto END;
                break;
        }
    }

    /* Verify mandatory options */
    if ((NULL == mac_address) || (NULL == artifact_name) || (NULL == device_type)) {
        ret = EXIT_FAILURE;
        goto END;
    }

    /* Initialize mender-mcu-client events cond and mutex */
    if (0 != pthread_cond_init(&mender_client_events_cond, NULL)) {
        /* Unable to initialize cond */
        ret = EXIT_FAILURE;
        goto END;
    }
    if (0 != pthread_mutex_init(&mender_client_events_mutex, NULL)) {
        /* Unable to initialize mutex */
        pthread_cond_destroy(&mender_client_events_cond);
        ret = EXIT_FAILURE;
        goto END;
    }

    /* Initialize mender-client */
    mender_client_config_t    mender_client_config    = { .mac_address                  = mac_address,
                                                    .artifact_name                = artifact_name,
                                                    .device_type                  = device_type,
                                                    .host                         = NULL,
                                                    .tenant_token                 = tenant_token,
                                                    .authentication_poll_interval = 0,
                                                    .update_poll_interval         = 0,
                                                    .recommissioning              = false };
    mender_client_callbacks_t mender_client_callbacks = { .authentication_success = authentication_success_cb,
                                                          .authentication_failure = authentication_failure_cb,
                                                          .deployment_status      = deployment_status_cb,
                                                          .ota_begin              = mender_ota_begin,
                                                          .ota_write              = mender_ota_write,
                                                          .ota_abort              = mender_ota_abort,
                                                          .ota_end                = mender_ota_end,
                                                          .ota_set_boot_partition = mender_ota_set_boot_partition,
                                                          .restart                = restart_cb };

    mender_client_init(&mender_client_config, &mender_client_callbacks);

    /* Initialize mender add-ons */
#ifdef CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE
    mender_configure_config_t    mender_configure_config    = { .refresh_interval = 0 };
    mender_configure_callbacks_t mender_configure_callbacks = {
#ifndef CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE
        .config_updated = config_updated_cb,
#endif /* CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE */
    };
    mender_configure_init(&mender_configure_config, &mender_configure_callbacks);
#endif /* CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE */
#ifdef CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY
    mender_inventory_config_t mender_inventory_config = { .refresh_interval = 0 };
    mender_inventory_init(&mender_inventory_config);
#endif /* CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY */

    /* Wait for mender-mcu-client events */
    pthread_mutex_lock(&mender_client_events_mutex);
    pthread_cond_wait(&mender_client_events_cond, &mender_client_events_mutex);
    pthread_mutex_unlock(&mender_client_events_mutex);

    /* Release mender add-ons */
#ifdef CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY
    mender_inventory_exit();
#endif /* CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY */
#ifdef CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE
    mender_configure_exit();
#endif /* CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE */

    /* Release mender-client */
    mender_client_exit();

END:

    /* Release memory */
    if (NULL != mac_address) {
        free(mac_address);
    }
    if (NULL != artifact_name) {
        free(artifact_name);
    }
    if (NULL != device_type) {
        free(device_type);
    }
    if (NULL != tenant_token) {
        free(tenant_token);
    }

    return ret;
}
