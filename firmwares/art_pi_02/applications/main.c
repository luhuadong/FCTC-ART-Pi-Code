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
#include "dhtxx.h"

#define LED_PIN GET_PIN(I, 8)
#define DATA_PIN GET_PIN(I, 5)

int main(void)
{
    rt_uint32_t count = 10;
    rt_device_t temp_sensor = RT_NULL;
    struct rt_sensor_data sensor_data;

    rt_kprintf("FCTC Demo 02\n");
    rt_kprintf("pin => %d\n", DATA_PIN);

    temp_sensor = rt_device_find("temp_dht");
    rt_device_open(temp_sensor, RT_DEVICE_FLAG_RDWR);

    while(count--)
    {
        rt_device_read(temp_sensor, 0, &sensor_data, 1);
        rt_kprintf("[%d] Temp: %d\n", sensor_data.timestamp, sensor_data.data.temp);
        rt_thread_mdelay(2000);
    }

    rt_device_close(temp_sensor);
    return RT_EOK;
}

static int rt_hw_dht_port(void)
{
    struct rt_sensor_config cfg;

    cfg.intf.type = RT_SENSOR_INTF_ONEWIRE;
    cfg.intf.user_data = (void *)DATA_PIN;
    rt_hw_dht_init("dht", &cfg);

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(rt_hw_dht_port);

#include "stm32h7xx.h"
static int vtor_config(void)
{
    /* Vector Table Relocation in Internal QSPI_FLASH */
    SCB->VTOR = QSPI_BASE;
    return 0;
}
INIT_BOARD_EXPORT(vtor_config);


