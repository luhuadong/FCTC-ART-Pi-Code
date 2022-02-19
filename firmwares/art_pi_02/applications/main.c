/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-03-02     Rudy Lo      first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_common.h"
#include "dhtxx.h"
#include "gp2y10.h"
#include "sgp30.h"

#define LOG_TAG     "main"
#define LOG_LVL     LOG_LVL_DBG
#include <ulog.h>

#define DHT22_DATA_PIN           GET_PIN(I, 5)
#define GP2Y10_ILED_PIN          GET_PIN(H, 2)
#define GP2Y10_AOUT_PIN          GET_PIN(B, 1)
#define SGP30_I2C_BUS_NAME       "i2c1"

static rt_mutex_t lock = RT_NULL;

static void sensor_read_entry(void *device_name)
{
    rt_device_t sensor = RT_NULL;
    struct rt_sensor_data sensor_data;

    sensor = rt_device_find(device_name);

    if (rt_device_open(sensor, RT_DEVICE_FLAG_RDWR)) {
        rt_kprintf("Open %s sensor device failed.\n", device_name);
        return;
    }

    while (1) {
        rt_device_read(sensor, 0, &sensor_data, 1);

        rt_mutex_take(lock, RT_WAITING_FOREVER);

        switch (sensor_data.type) {
        case RT_SENSOR_CLASS_TEMP:
            rt_kprintf("[%d] Temp: %d 'C\n", sensor_data.timestamp, sensor_data.data.temp/10);
            break;
        case RT_SENSOR_CLASS_HUMI:
            rt_kprintf("[%d] Humi: %d RH%\n", sensor_data.timestamp, sensor_data.data.humi/10);
            break;
        case RT_SENSOR_CLASS_DUST:
            rt_kprintf("[%d] Dust: %d ug/m3\n", sensor_data.timestamp, sensor_data.data.dust);
            break;
        case RT_SENSOR_CLASS_TVOC:
            rt_kprintf("[%d] TVOC: %d ppb\n", sensor_data.timestamp, sensor_data.data.tvoc);
            break;
        case RT_SENSOR_CLASS_ECO2:
            rt_kprintf("[%d] eCO2: %d ppm\n", sensor_data.timestamp, sensor_data.data.eco2);
            break;
        default: break;
        }

        rt_mutex_release(lock);

        rt_thread_mdelay(2000);
    }

    rt_device_close(sensor);
}

int main(void)
{
    lock = rt_mutex_create("lock", RT_IPC_FLAG_PRIO);

    rt_thread_t tid_1, tid_2, tid_3, tid_4, tid_5;

    tid_1 = rt_thread_create("temp_th", sensor_read_entry, "temp_dht", 1024, 15, 20);
    tid_2 = rt_thread_create("humi_th", sensor_read_entry, "humi_dht", 1024, 15, 20);
    tid_3 = rt_thread_create("dust_th", sensor_read_entry, "dust_gp2", 1024, 15, 20);
    tid_4 = rt_thread_create("tvoc_th", sensor_read_entry, "tvoc_sg3", 1024, 15, 20);
    tid_5 = rt_thread_create("eco2_th", sensor_read_entry, "eco2_sg3", 1024, 15, 20);

    if (tid_1) rt_thread_startup(tid_1);
    if (tid_2) rt_thread_startup(tid_2);
    if (tid_3) rt_thread_startup(tid_3);
    if (tid_4) rt_thread_startup(tid_4);
    if (tid_5) rt_thread_startup(tid_5);

    return RT_EOK;
}

static int rt_hw_dht_port(void)
{
    struct rt_sensor_config cfg;

    cfg.intf.type = RT_SENSOR_INTF_ONEWIRE;
    cfg.intf.user_data = (void *)DHT22_DATA_PIN;
    rt_hw_dht_init("dht", &cfg);

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(rt_hw_dht_port);

static int rt_hw_gp2y10_port(void)
{
    struct gp2y10_device gp2y10_dev;
    struct rt_sensor_config cfg;

    gp2y10_dev.iled_pin = GP2Y10_ILED_PIN;
    gp2y10_dev.aout_pin = GP2Y10_AOUT_PIN;

    cfg.intf.user_data = (void *)&gp2y10_dev;
    rt_hw_gp2y10_init("gp2", &cfg);

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(rt_hw_gp2y10_port);

static int rt_hw_sgp30_port(void)
{
    struct rt_sensor_config cfg;

    cfg.intf.type = RT_SENSOR_INTF_I2C;
    cfg.intf.dev_name = SGP30_I2C_BUS_NAME;
    rt_hw_sgp30_init("sg3", &cfg);

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(rt_hw_sgp30_port);

#include "stm32h7xx.h"
static int vtor_config(void)
{
    /* Vector Table Relocation in Internal QSPI_FLASH */
    SCB->VTOR = QSPI_BASE;
    return 0;
}
INIT_BOARD_EXPORT(vtor_config);
