/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-28     Rudy Lo      The First version
 */
#ifndef LV_PORT_INDEV_H
#define LV_PORT_INDEV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lv_hal_indev.h>

extern lv_indev_t * button_indev;

void lv_port_indev_init(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif