/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-02-02     Rudy Lo      first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h> /* 当需要使用文件操作时，需要包含这个头文件 */
#include "drv_common.h"
#include "pmsxx.h"
#include <easyflash.h>

//#define FILE_NAME      "/sdcard/sensor.csv"
#define FILE_NAME      "/flash/sensor.csv"
#define UART_NAME      "uart6"

static void read_sensor_data_to_sdcard(void)
{
    int fd = 0, ret = 0, count = 10;
    pms_device_t sensor = RT_NULL;
    struct pms_response resp;
    char buf[80] = {0};

    rt_kprintf("Read sensor data to SD card\n");
    fd = open(FILE_NAME, O_WRONLY | O_CREAT);
    if (fd < 0) {
        rt_kprintf("Open %s failed.\n", FILE_NAME);
        return;
    }

    char header[] = "Timestamp,PM2.5,HCHO,Temp,Humi\n";
    write(fd, header, strlen(header));  /* 写入表头 */

    sensor = pms_create(UART_NAME);
    if (!sensor) {
        rt_kprintf("Init PMS5003ST sensor failed!\n");
        close(fd);
        return;
    }

    pms_set_mode(sensor, PMS_MODE_NORMAL);
    pms_set_mode(sensor, PMS_MODE_PASSIVE);

    while (count--) {
        memset(&resp, 0, sizeof(resp));
        memset(buf, 0, sizeof(buf));

        rt_kprintf("Request %d\n", count);
        ret = pms_read(sensor, &resp, sizeof(resp), rt_tick_from_millisecond(3000));

        if (ret != sizeof(resp)) {
            rt_kprintf("Read error\n");
        }

        /* 按 CSV 格式写入传感器数据 */
        rt_snprintf(buf, sizeof(buf), "%ld,%d,%d,%d,%d\n",
                    time(RT_NULL), resp.PM2_5_atm, resp.hcho, resp.temp, resp.humi);
        write(fd, buf, strlen(buf));
        rt_thread_mdelay(3000);
    }

    rt_kprintf("Sensor data stored in %s\n", FILE_NAME);
    pms_delete(sensor);
    close(fd);
}
MSH_CMD_EXPORT(read_sensor_data_to_sdcard, Read sensor data to sdcard);

static void test_env(void)
{
    rt_uint32_t boot_times = 0;
    size_t value_len = 0, ret = 0;

    /* 从环境变量中获取启动次数 */
    ret = ef_get_env_blob("boot_times", &boot_times, sizeof(boot_times), &value_len);

    /* 获取启动次数是否失败 */
    if (ret != sizeof(boot_times)) {
        rt_kprintf("Not found 'boot_times' in ENV.\n");
        boot_times = 0;
    }

    /* 启动次数加 1 */
    boot_times++;

    rt_kprintf("=======================================\n");
    rt_kprintf("The system now boot %d times\n", boot_times);
    rt_kprintf("=======================================\n");

    /* 保存开机次数的值 */
    ef_set_env_blob("boot_times", &boot_times, sizeof(boot_times));
    ef_save_env();
}

int main(void)
{
    if (easyflash_init() == EF_NO_ERR)
    {
        /* 演示环境变量功能 */
        test_env();
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

