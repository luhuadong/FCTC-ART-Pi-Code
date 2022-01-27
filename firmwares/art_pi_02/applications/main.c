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
#include "gp2y10.h"

#define LED_PIN GET_PIN(I, 8)
#define DATA_PIN GET_PIN(I, 5)

#define GP2Y10_ILED_PIN          GET_PIN(C, 7)
#define GP2Y10_AOUT_PIN          GET_PIN(B, 1)

int main(void)
{
    rt_uint32_t count = 3;
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


static void read_dust_entry(void *args)
{
    rt_device_t dust_dev = RT_NULL;
    struct rt_sensor_data sensor_data;

    dust_dev = rt_device_find(args);
    if (dust_dev == RT_NULL)
    {
        rt_kprintf("Can't find dust device.\n");
        return;
    }

    if (rt_device_open(dust_dev, RT_DEVICE_FLAG_RDWR))
    {
        rt_kprintf("Open dust device failed.\n");
        return;
    }

    while(1)
    {
        if (1 != rt_device_read(dust_dev, 0, &sensor_data, 1))
        {
            rt_kprintf("Read dust data failed.\n");
            continue;
        }
        rt_kprintf("[%d] Dust: %d\n", sensor_data.timestamp, sensor_data.data.dust);

        rt_thread_mdelay(2000);
    }

    rt_device_close(dust_dev);
}

static int gp2y10_read_sample(void)
{
    rt_thread_t dust_thread;

    dust_thread = rt_thread_create("dust_th", read_dust_entry,
                                   "dust_gp2", 1024,
                                    RT_THREAD_PRIORITY_MAX / 2, 20);

    if (dust_thread)
        rt_thread_startup(dust_thread);
}
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(gp2y10_read_sample, read gp2y10 sensor data);
#endif

static int rt_hw_gp2y10_port(void)
{
    struct gp2y10_device gp2y10_dev;
    struct rt_sensor_config cfg;

    gp2y10_dev.iled_pin = GP2Y10_ILED_PIN;
    gp2y10_dev.aout_pin = GP2Y10_AOUT_PIN;

    //cfg.intf.type = RT_SENSOR_INTF_ADC;
    cfg.intf.user_data = (void *)&gp2y10_dev;
    rt_hw_gp2y10_init("gp2", &cfg);

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(rt_hw_gp2y10_port);

#include "stm32h7xx.h"
static int vtor_config(void)
{
    /* Vector Table Relocation in Internal QSPI_FLASH */
    SCB->VTOR = QSPI_BASE;
    return 0;
}
INIT_BOARD_EXPORT(vtor_config);


