/**
 * @file      mender-addon.h
 * @brief     Mender MCU addon implementation
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

#ifndef __MENDER_ADDON_H__
#define __MENDER_ADDON_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-utils.h"

/**
 * @brief Mender add-on instance
 */
typedef struct {
    mender_err_t (*init)(void *, void *); /**< Invoked to initialize the add-on */
    mender_err_t (*activate)(void);       /**< Invoked to activate the add-on */
    mender_err_t (*deactivate)(void);     /**< Invoked to deactivate the add-on */
    mender_err_t (*exit)(void);           /**< Invoked to cleanup the add-on */
} mender_addon_instance_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_ADDON_H__ */
