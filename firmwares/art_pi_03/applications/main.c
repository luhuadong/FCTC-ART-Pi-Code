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
#include "pmsxx.h"

#define UART_NAME   "uart6"

int main(void)
{
    rt_uint16_t loop = 10;
    struct pms_response resp;
    pms_device_t sensor = pms_create(UART_NAME);
    if (!sensor)
    {
        rt_kprintf("(PMS) Init failed\n");
        return;
    }

    while (!pms_is_ready(sensor))
    {
        rt_thread_mdelay(1000);
    }

    pms_set_mode(sensor, PMS_MODE_NORMAL);
    pms_set_mode(sensor, PMS_MODE_PASSIVE);

    while (1)
    {
        rt_kprintf("\n[%d] Request...\n", loop);
        pms_read(sensor, &resp, sizeof(resp), rt_tick_from_millisecond(3000));
        pms_show_response(&resp);
        rt_thread_mdelay(3000);
    }

    rt_kprintf("(PMS) Finished!\n");
    pms_delete(sensor);
}

#include "stm32h7xx.h"
static int vtor_config(void)
{
    /* Vector Table Relocation in Internal QSPI_FLASH */
    SCB->VTOR = QSPI_BASE;
    return 0;
}
INIT_BOARD_EXPORT(vtor_config);

