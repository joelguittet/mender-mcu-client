/**
 * @file      mender-configure-api.h
 * @brief     Mender MCU configure add-on API implementation
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

#ifndef __MENDER_CONFIGURE_API_H__
#define __MENDER_CONFIGURE_API_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-utils.h"

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE

#ifndef CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE

/**
 * @brief Download configure data of the device from the mender-server
 * @param configuration Mender configuration key/value pairs table, ends with a NULL/NULL element, NULL if not defined
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_configure_api_download_configuration_data(mender_keystore_t **configuration);

#endif /* CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE */

/**
 * @brief Publish configure data of the device to the mender-server
 * @param configuration Mender configuration key/value pairs table, must end with a NULL/NULL element, NULL if not defined
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_configure_api_publish_configuration_data(mender_keystore_t *configuration);

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_CONFIGURE_API_H__ */
