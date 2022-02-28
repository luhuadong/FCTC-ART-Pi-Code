/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-07-25     Rudy Lo     first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_common.h"
#include <lvgl.h>
#include "mb.h"
#include "mb_m.h"

#define DBG_TAG        "main"
#define DBG_LVL        DBG_INFO
#include <rtdbg.h>

#define SLAVE_ADDR         0x01         /* 从设备地址 */
#define PORT_NUM           5            /* 串口号（UART5） */
#define PORT_BAUDRATE      9600         /* 波特率 */
#define PORT_PARITY        MB_PAR_NONE  /* 无奇偶校验 */
#define MB_POLL_CYCLE_MS   500          /* Modbus 主站轮询周期 */

#ifndef LV_THREAD_STACK_SIZE
#define LV_THREAD_STACK_SIZE 4096
#endif

#ifndef LV_THREAD_PRIO
#define LV_THREAD_PRIO (RT_THREAD_PRIORITY_MAX * 2 / 3)
#endif

static rt_sem_t lvgl_sem = RT_NULL;

static int turn_on_coil(rt_uint16_t num);
static int turn_off_coil(rt_uint16_t num);

static void sw_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e); /* 获取当前部件(对象)触发的事件代码 */
    lv_obj_t * sw = lv_event_get_target(e);      /* 获取触发事件的部件(对象) */
    int num = lv_event_get_user_data(e);         /* 获取部件(对象)的用户数据 */

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        rt_kprintf("Relay %d switch %s\n", num, lv_obj_has_state(sw, LV_STATE_CHECKED) ? "On" : "Off");

        if (lv_obj_has_state(sw, LV_STATE_CHECKED))
        {
            turn_on_coil(num);
        }
        else
        {
            turn_off_coil(num);
        }
    }
}

static lv_obj_t *value_temp;
static lv_obj_t *value_humi;
static lv_obj_t *value_pm25;
static lv_obj_t *value_hcho;

void create_ui_page(void)
{
    // 定义行和列描述符的数组，数组的最后一个元素必须是LV_GRID_TEMPLATE_LAST
    static lv_coord_t col_dsc[] = { 80, 160, 80, 80, LV_GRID_TEMPLATE_LAST }; // 4列，分别指定每列的宽度
    static lv_coord_t row_dsc[] = { 60, 60, 60, 60, LV_GRID_TEMPLATE_LAST };  // 4行，分别指定每行的高度

    lv_obj_t * cont = lv_obj_create(lv_scr_act()); // 基于屏幕创建一个容器对象

    lv_obj_set_size(cont, 480, 320); // 设置对象宽度和高度
    lv_obj_center(cont);             // 设置对象居屏幕中间显示
    lv_obj_set_layout(cont, LV_LAYOUT_GRID); //设置对象使用网格布局
    lv_obj_set_style_grid_column_dsc_array(cont, col_dsc, 0); // 设置对象网格布局行描述数组
    lv_obj_set_style_grid_row_dsc_array(cont, row_dsc, 0);    // 设置对象网格布局列描述数组

    lv_obj_t * obj = NULL;
    int value_default = 0;

    const char *label_names[] = { "Temp", "Humi", "PM2.5", "HCHO" };

    for (int i = 0; i < 4; i++)
    {
        obj = lv_label_create(cont);
        lv_label_set_text_fmt(obj, "%s", label_names[i]);
        lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, i, 1);
    }

    value_temp = lv_label_create(cont);
    value_humi = lv_label_create(cont);
    value_pm25 = lv_label_create(cont);
    value_hcho = lv_label_create(cont);

    lv_label_set_text_fmt(value_temp, "%d  'C", value_default);
    lv_label_set_text_fmt(value_humi, "%d  %RH", value_default);
    lv_label_set_text_fmt(value_pm25, "%d  ug/m3", value_default);
    lv_label_set_text_fmt(value_hcho, "%d  mg/m3", value_default);

    lv_obj_set_grid_cell(value_temp, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_grid_cell(value_humi, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_set_grid_cell(value_pm25, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 2, 1);
    lv_obj_set_grid_cell(value_hcho, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 3, 1);

    for (int i = 0; i < 4; i++)
    {
        obj = lv_label_create(cont);
        lv_label_set_text_fmt(obj, "Relay%d", i + 1);
        lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_CENTER, i, 1);
    }

    for (int i = 0; i < 4; i++)
    {
        obj = lv_switch_create(cont);
        lv_obj_set_height(obj, 40);
        lv_obj_add_event_cb(obj, sw_event_cb, LV_EVENT_VALUE_CHANGED, (void *) i);
        lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_STRETCH, 3, 1, LV_GRID_ALIGN_CENTER, i, 1);
    }
}

static void lvgl_thread(void *parameter)
{
    create_ui_page();
    rt_sem_release(lvgl_sem);

    /* handle the tasks of LVGL */
    while (1)
    {
        lv_task_handler();
        rt_thread_mdelay(10);
    }
}

static void update_thread(void *parameter)
{
    rt_err_t result = 0;

    result = rt_sem_take(lvgl_sem, RT_WAITING_FOREVER);
    if (result == RT_EOK) {
        rt_kprintf("get lvgl semaphore\n");
        rt_sem_delete(lvgl_sem);
    }

    while (1)
    {
        lv_label_set_text_fmt(value_temp, "%d  'C", random()%100-20);
        lv_label_set_text_fmt(value_humi, "%u  %RH", random()%100);
        lv_label_set_text_fmt(value_pm25, "%u  ug/m3", random()%500);
        lv_label_set_text_fmt(value_hcho, "0.%03u  mg/m3", random()%1000);

        rt_thread_mdelay(1000);
    }
}

static void mb_master_poll(void *parameter)
{
    eMBMasterInit(MB_RTU, PORT_NUM, PORT_BAUDRATE, PORT_PARITY);
    eMBMasterEnable();

    while (1)
    {
        eMBMasterPoll();
        rt_thread_mdelay(MB_POLL_CYCLE_MS);
    }
}

static int turn_on_coil(rt_uint16_t num)
{
    eMBMasterReqErrCode error_code = MB_MRE_NO_ERR;

    error_code = eMBMasterReqWriteCoil(SLAVE_ADDR, num, 0xFF00, rt_tick_from_millisecond(3000));

    if (error_code != MB_MRE_NO_ERR)
    {
        rt_kprintf("Error code: %d\n", error_code);
        return -RT_ERROR;
    }
    return RT_EOK;
}

static int turn_off_coil(rt_uint16_t num)
{
    eMBMasterReqErrCode error_code = MB_MRE_NO_ERR;

    error_code = eMBMasterReqWriteCoil(SLAVE_ADDR, num, 0x0000, rt_tick_from_millisecond(3000));

    if (error_code != MB_MRE_NO_ERR)
    {
        rt_kprintf("Error code: %d\n", error_code);
        return -RT_ERROR;
    }
    return RT_EOK;
}

int main(void)
{
    rt_thread_t lvgl_tid, update_tid, modbus_tid;

    lvgl_sem = rt_sem_create("lvgl_sem", 0, RT_IPC_FLAG_PRIO);
    if (lvgl_sem == RT_NULL) {
        rt_kprintf("create semaphore failed.\n");
        return -1;
    }

    lvgl_tid = rt_thread_create("LVGL", lvgl_thread, RT_NULL, 4096, 22, 0);
    if (lvgl_tid)
        rt_thread_startup(lvgl_tid);

    update_tid = rt_thread_create("update", update_thread, RT_NULL, 1024, 8, 0);
    if (update_tid)
        rt_thread_startup(update_tid);

    modbus_tid = rt_thread_create("md_poll", mb_master_poll, RT_NULL, 512, 10, 10);
    if (modbus_tid)
        rt_thread_startup(modbus_tid);

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

