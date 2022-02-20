/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-06-02     Rudy Lo      first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_common.h"
#include "pmsxx.h"

#define UART_NAME      "uart6"

#define PASSIVE_MODE   1  /* 0: Sensor -> MCU, 1: MCU -> Sensor */

int main(void)
{
    int ret = 0;
    struct pms_response resp;

    pms_device_t sensor = pms_create(UART_NAME);

    if (!sensor) {
        rt_kprintf("Init PMS5003ST sensor failed!\n");
        return -1;
    }

#if 0
    while (!pms_is_ready(sensor)) {
        rt_kprintf("wait ready...\n");
        rt_thread_mdelay(1000);
    }
#endif

#if 1
    pms_set_mode(sensor, PMS_MODE_NORMAL);
#if PASSIVE_MODE
    pms_set_mode(sensor, PMS_MODE_PASSIVE);
#else
    pms_set_mode(sensor, PMS_MODE_ACTIVE);
#endif
#endif

    while (1)
    {
        rt_memset(&resp, 0, sizeof(resp));

#if PASSIVE_MODE
        rt_kprintf("\nRequest...\n");
        ret = pms_read(sensor, &resp, sizeof(resp), rt_tick_from_millisecond(3000));
#else
        rt_kprintf("\nWait...\n");
        ret = pms_wait(sensor, &resp, sizeof(resp));
#endif

        if (ret == sizeof(resp)) {
            pms_show_response(&resp);
        }
        else {
            rt_kprintf("Read error\n");
        }

        rt_thread_mdelay(3000);
    }

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

