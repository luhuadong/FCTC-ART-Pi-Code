/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-02-26     Rudy Lo      the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_common.h"
#include "touch.h"
#include "ft6236.h"

#define FT6236_I2C_BUS        "i2c2"
#define FT6236_REST_PIN       GET_PIN(A, 3)

void touch_sample_entry(void *parameter)
{
    struct rt_touch_data *data;
    rt_device_t touch = (rt_device_t) parameter;

    data = (struct rt_touch_data *) rt_calloc(1, sizeof(struct rt_touch_data));

    struct rt_touch_info info;
    rt_device_control(touch, RT_TOUCH_CTRL_GET_INFO, &info);
    rt_kprintf("point_num  :%d\n", info.point_num);
    rt_kprintf("range_x    :%d\n", info.range_x);
    rt_kprintf("range_y    :%d\n", info.range_y);

    while (1)
    {
        rt_device_read(touch, 0, data, 1);

        switch (data->event)
        {
        case RT_TOUCH_EVENT_DOWN:
            rt_kprintf("down x: %03d y: %03d\n", data->x_coordinate, data->y_coordinate);
            break;
        /*case RT_TOUCH_EVENT_MOVE:
            rt_kprintf("move x: %03d y: %03d\n", data->x_coordinate, data->y_coordinate);
            break;*/
        case RT_TOUCH_EVENT_UP:
            rt_kprintf("up   x: %03d y: %03d\n", data->x_coordinate, data->y_coordinate);
            break;
        default:
            break;
        }

        rt_thread_delay(10);
    }
}

int ft6236_sample(void)
{
    rt_thread_t tid;
    rt_device_t touch;

    struct rt_touch_config cfg = { .dev_name = FT6236_I2C_BUS, };

    rt_hw_ft6236_init("touch", &cfg, FT6236_REST_PIN);
    touch = rt_device_find("touch");

    if (RT_EOK == rt_device_open(touch, RT_DEVICE_FLAG_RDONLY))
    {
        tid = rt_thread_create("touch", touch_sample_entry, (void *) touch, 1024, 10, 20);
        if (tid)
            rt_thread_startup(tid);
    }

    return RT_EOK;
}
INIT_APP_EXPORT(ft6236_sample);
