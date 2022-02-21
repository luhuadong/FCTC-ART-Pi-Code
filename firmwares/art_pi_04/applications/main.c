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

#include "mb.h"
#include "mb_m.h"

#define SLAVE_ADDR         0x01
#define PORT_NUM           5
#define PORT_BAUDRATE      9600
#define PORT_PARITY        MB_PAR_NONE
#define MB_POLL_CYCLE_MS   500

static int turn_on_coil(rt_uint16_t num)
{
    eMBMasterReqErrCode error_code = MB_MRE_NO_ERR;

    error_code = eMBMasterReqWriteCoil(SLAVE_ADDR, num, 0xFF00, rt_tick_from_millisecond(3000));

    if (error_code != MB_MRE_NO_ERR) {
        rt_kprintf("Error code: %d\n", error_code);
        return -RT_ERROR;
    }
    return RT_EOK;
}

static int turn_off_coil(rt_uint16_t num)
{
    eMBMasterReqErrCode error_code = MB_MRE_NO_ERR;

    error_code = eMBMasterReqWriteCoil(SLAVE_ADDR, num, 0x0000, rt_tick_from_millisecond(3000));

    if (error_code != MB_MRE_NO_ERR) {
        rt_kprintf("Error code: %d\n", error_code);
        return -RT_ERROR;
    }
    return RT_EOK;
}

static void modbus_set_coil(int argc, char **argv)
{
    if (argc < 3) {
        rt_kprintf("Usage:\n");
        rt_kprintf("  %s on  <num>\n", argv[0]);
        rt_kprintf("  %s off <num>\n", argv[0]);
        return;
    }

    rt_uint16_t num = atoi(argv[2]);
    if (num > 3) {
        rt_kprintf("Invaild parameters\n");
        return;
    }

    if (0 == rt_strncmp("on", argv[1], 2)) {
        eMBMasterReqWriteCoil(SLAVE_ADDR, num, 0xFF00, rt_tick_from_millisecond(3000));
        //turn_on_coil(num);
    }
    else if (0 == rt_strncmp("off", argv[1], 3)) {
        eMBMasterReqWriteCoil(SLAVE_ADDR, num, 0x0000, rt_tick_from_millisecond(3000));
        //turn_off_coil(num);
    }
    else {
        rt_kprintf("Unknown commands\n");
    }
}
MSH_CMD_EXPORT(modbus_set_coil, Modbus master set coil sample);

static void mb_master_poll(void *parameter)
{
    eMBMasterInit(MB_RTU, PORT_NUM, PORT_BAUDRATE, PORT_PARITY);
    eMBMasterEnable();

    while (1) {
        eMBMasterPoll();
        rt_thread_mdelay(MB_POLL_CYCLE_MS);
    }
}

int main(void)
{
    rt_thread_t tid = RT_NULL;
    tid = rt_thread_create("md_poll", mb_master_poll, RT_NULL, 512, 10, 10);

    if (tid == RT_NULL) {
        rt_kprintf("Create mb_master_poll thread failed!\n");
        return RT_ERROR;
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
