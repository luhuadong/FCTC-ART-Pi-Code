/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-09-02     RT-Thread    first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_common.h"
#include <lvgl.h>

#define DBG_TAG    "main"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

#ifndef LV_THREAD_STACK_SIZE
#define LV_THREAD_STACK_SIZE 4096
#endif

#ifndef LV_THREAD_PRIO
#define LV_THREAD_PRIO (RT_THREAD_PRIORITY_MAX * 2 / 3)
#endif

extern void lv_demo_music(void);

static void btn_toggle_event_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);        // 获取触发事件的部件(对象)
    lv_event_code_t code = lv_event_get_code(e);    // 获取当前部件(对象)触发的事件代码

    switch(code){
        case LV_EVENT_VALUE_CHANGED:
            rt_kprintf("LV_EVENT_VALUE_CHANGED\n");
            break;
        default:
            //rt_kprintf("NONE\n");
            break;
    }
}

void my_lvgl_demo(void)
{
#if 1
    lv_obj_t *label = lv_label_create(lv_scr_act());
    //lv_obj_set_size(label, LV_PCT(80), LV_PCT(80));
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(label, "Hello, World!");

#else
    lv_obj_t * obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(obj, LV_PCT(30), LV_PCT(15));  // 使用百分比设置大小
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * btn = lv_btn_create(lv_scr_act());
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x1e1e1e), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_size(btn, 60, 60);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_add_event_cb(btn, btn_toggle_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
#endif
}

static void lvgl_thread(void *parameter)
{
    my_lvgl_demo();

    /* handle the tasks of LVGL */
    while(1)
    {
        lv_task_handler();
        rt_thread_mdelay(10);
    }
}


int main(void)
{
    rt_thread_t tid;

        tid = rt_thread_create("LVGL", lvgl_thread, RT_NULL, LV_THREAD_STACK_SIZE, LV_THREAD_PRIO, 0);
        if(tid == RT_NULL)
        {
            LOG_E("Fail to create 'LVGL' thread");
        }
        rt_thread_startup(tid);

    return RT_EOK;
}

#include "stm32h7xx.h"
static int vtor_config(void)
{
    /* Vector Table Relocation in Internal QSPI_FLASH */
    SCB->VTOR = QSPI_BASE;
    return 0;
}
INIT_BOARD_EXPORT(vtor_config);


