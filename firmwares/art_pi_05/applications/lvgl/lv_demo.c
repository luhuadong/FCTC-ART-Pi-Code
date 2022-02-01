/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2021-10-17     Meco Man      First version
 */
#include <rtthread.h>
#include <lvgl.h>
#include "lv_port_indev.h"
#include "demo/lv_demo_calendar.h"
#define DBG_TAG    "LVGL.demo"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

#ifndef LV_THREAD_STACK_SIZE
#define LV_THREAD_STACK_SIZE 4096
#endif

#ifndef LV_THREAD_PRIO
#define LV_THREAD_PRIO (RT_THREAD_PRIORITY_MAX * 2 / 3)
#endif

static void lvgl_thread(void *parameter)
{
#if 0
    /*assign buttons to coordinates*/
    const lv_point_t points_array[] =  {{200,35},{0,0},{70,35},{0,0}};
    lv_indev_set_button_points(button_indev, points_array);

    /* display demo; you may replace with your LVGL application at here */
    lv_demo_calendar();
#else
    extern void lv_demo_music(void);
    lv_demo_music();
#endif

    /* handle the tasks of LVGL */
    while(1)
    {
        lv_task_handler();
        rt_thread_mdelay(10);
    }
}

static int lvgl_demo_init(void)
{
    rt_thread_t tid;

    tid = rt_thread_create("LVGL", lvgl_thread, RT_NULL, LV_THREAD_STACK_SIZE, LV_THREAD_PRIO, 0);
    if(tid == RT_NULL)
    {
        LOG_E("Fail to create 'LVGL' thread");
    }
    rt_thread_startup(tid);

    return 0;
}
MSH_CMD_EXPORT(lvgl_demo_init, run lvgl demo)
//INIT_APP_EXPORT(lvgl_demo_init);
