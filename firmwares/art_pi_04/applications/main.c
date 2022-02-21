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

#include "mb.h"
#include "mb_m.h"

#define SLAVE_ADDR      0x01
#define PORT_NUM        5
#define PORT_BAUDRATE   9600

#define PORT_PARITY     MB_PAR_NONE

#define MB_POLL_THREAD_PRIORITY  (RT_THREAD_PRIORITY_MAX / 4)
#define MB_SEND_THREAD_PRIORITY  (RT_THREAD_PRIORITY_MAX - 1)  /* Why? */

#define MB_SEND_REG_START  2
#define MB_SEND_REG_NUM    2

#define MB_POLL_CYCLE_MS   500

static void mb_master_samlpe(int argc, char **argv)
{
    eMBMasterReqErrCode error_code = MB_MRE_NO_ERR;

    if (argc < 3) {
        rt_kprintf("Modbus master test sample\n");
        rt_kprintf("Usage:\n");
        rt_kprintf("  %s read <num>\n", argv[0]);
        rt_kprintf("  %s turn_on <num>\n", argv[0]);
        rt_kprintf("  %s turn_off <num>\n", argv[0]);
        return;
    }

    rt_uint16_t num = atoi(argv[2]);
    if (num > 3) {
        rt_kprintf("Invaild parameters\n");
        return;
    }

    if (0 == rt_strncmp("read", argv[1], 4)) {

        rt_kprintf("Read coil %s\n", argv[2]);
        error_code = eMBMasterReqReadCoils(SLAVE_ADDR, num, 1, RT_WAITING_FOREVER);

#if 0
        UCHAR  pucFrame[16] = {0};
        USHORT usLen = 0;

        eMBMasterFuncReadCoils(pucFrame, &usLen);

        for (int i=0; i<sizeof(pucFrame); i++) {
            rt_kprintf("%02X ", pucFrame[i]);
        }
        rt_kprintf("\n");
#else
        if (error_code == MB_MRE_NO_ERR) {
            rt_kprintf("Error code: %d\n", error_code);
            //extern UCHAR ucMCoilBuf[MB_MASTER_TOTAL_SLAVE_NUM][M_COIL_NCOILS/8];
            //extern ucMCoilBuf;
            extern UCHAR ucMCoilBuf[16][8];
            rt_kprintf("Coil state: %d\n", ucMCoilBuf[0][1]);
        }

#endif
    }
    else if (0 == rt_strncmp("turn_on", argv[1], 7)) {
        rt_kprintf("Turn on coil %s\n", argv[2]);
        error_code = eMBMasterReqWriteCoil(SLAVE_ADDR, num, 0xFF00, RT_WAITING_FOREVER);
    }
    else if (0 == rt_strncmp("turn_off", argv[1], 8)) {
        rt_kprintf("Turn off coil %s\n", argv[2]);
        error_code = eMBMasterReqWriteCoil(SLAVE_ADDR, num, 0x0000, RT_WAITING_FOREVER);
    }
    else {
        rt_kprintf("Unknown commands\n");
    }

    /* Record the number of errors */
    if (error_code != MB_MRE_NO_ERR) {
        rt_kprintf("Error code: %d\n", error_code);
    }
}
MSH_CMD_EXPORT(mb_master_samlpe, run a modbus master sample);

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

int main(void)
{
    rt_thread_t tid1 = RT_NULL;

    tid1 = rt_thread_create("md_poll", mb_master_poll, RT_NULL, 512, MB_POLL_THREAD_PRIORITY, 10);
    if (tid1 != RT_NULL) {
        rt_thread_startup(tid1);
    } else {
        rt_kprintf("Create mb_master_poll thread failed!\n");
        return RT_ERROR;
    }

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

