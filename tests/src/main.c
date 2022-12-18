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

#include <stdio.h>

#include "mender-client.h"
#include "mender-ota.h"

/**
 * @brief Main function
 * @param argc Number of arguments
 * @param argv Arguments
 * @return Always returns 0
 */
int
main(int argc, char **argv) {

    /* Initialize mender-client */
    mender_client_config_t    mender_client_config    = { .mac_address                  = "00:11:22:33:44:55",
                                                    .artifact_name                = "artifact_name",
                                                    .device_type                  = "device_type",
                                                    .host                         = "https://hosted.mender.io",
                                                    .tenant_token                 = NULL,
                                                    .authentication_poll_interval = 0,
                                                    .inventory_poll_interval      = 0,
                                                    .update_poll_interval         = 0,
                                                    .restart_poll_interval        = 0,
                                                    .recommissioning              = false };
    mender_client_callbacks_t mender_client_callbacks = { .authentication_success = NULL,
                                                          .authentication_failure = NULL,
                                                          .deployment_status      = NULL,
                                                          .ota_begin              = mender_ota_begin,
                                                          .ota_write              = mender_ota_write,
                                                          .ota_abort              = mender_ota_abort,
                                                          .ota_end                = mender_ota_end,
                                                          .ota_set_boot_partition = mender_ota_set_boot_partition,
                                                          .restart                = NULL };

    mender_client_init(&mender_client_config, &mender_client_callbacks);

    /* Release mender-client */
    mender_client_exit();

    return 0;
}
