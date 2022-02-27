/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2022-02-03     Rudy Lo       First version
 */

#include <rtthread.h>
#include <lvgl.h>

#define LV_THREAD_STACK_SIZE 4096
#define LV_THREAD_PRIO (RT_THREAD_PRIORITY_MAX * 2 / 3)

void my_lvgl_demo(void)
{
    lv_obj_t *label = lv_label_create(lv_scr_act()); /* 创建一个标签 */
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);      /* 标签居中对齐 */
    lv_label_set_text(label, "Hello, World!");       /* 显示一串文字 */
}

static void lvgl_thread(void *parameter)
{
    my_lvgl_demo();            /* LVGL 界面程序 */

    while(1)
    {
        lv_task_handler();     /* 处理 LVGL 任务 */
        rt_thread_mdelay(10);
    }
}

static int lvgl_demo_init(void)
{
    rt_thread_t tid;

    /* 创建 LVGL 线程 */
    tid = rt_thread_create("LVGL", lvgl_thread, RT_NULL, LV_THREAD_STACK_SIZE, LV_THREAD_PRIO, 0);
    if(tid) rt_thread_startup(tid);

    return 0;
}
INIT_APP_EXPORT(lvgl_demo_init);
