/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TSDB samples.
 *
 * Time series log (like TSDB) feature samples source file.
 *
 * TSL is time series log, the TSDB saved many TSLs.
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <flashdb.h>
#include <string.h>
#include "drv_common.h"
#include "pmsxx.h"

#ifdef FDB_USING_TSDB

#define UART_NAME      "uart6"
#define FDB_LOG_TAG    "[tsdb]"

#ifdef FDB_USING_FILE_MODE
#define TSDB_PATH      "/flash/sensor.db"
#else
#ifdef FDB_USING_FAL_MODE
#define TSDB_PATH      "download"
#endif
#endif

struct env_status
{
    int pm25;
    int hcho;
    int temp;
    int humi;
};

static struct fdb_tsdb tsdb;
static rt_mutex_t tsdb_lock;

static void lock(fdb_db_t db)
{
    rt_mutex_take(tsdb_lock, RT_WAITING_FOREVER);
}

static void unlock(fdb_db_t db)
{
    rt_mutex_release(tsdb_lock);
}

static fdb_time_t get_time(void)
{
    return time(RT_NULL);
}

static int sensor_tsdb_init(void)
{
    int result = FDB_NO_ERR;
    tsdb_lock  = rt_mutex_create("tsdblock", RT_IPC_FLAG_PRIO);

    /* 设置 lock 和 unlock 函数 */
    fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_LOCK, lock);
    fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_UNLOCK, unlock);

    /* 初始化 TSDB */
    result = fdb_tsdb_init(&tsdb, "sensor", TSDB_PATH, get_time, 128, NULL);

    if (result != FDB_NO_ERR)
    {
        rt_kprintf("FlashDB TSDB init failed.\n");
        return -RT_ERROR;
    }
    rt_kprintf("FlashDB TSDB init success.\n");
    return RT_EOK;
}
INIT_APP_EXPORT(sensor_tsdb_init);

static void read_sensor_data_to_tsdb(void)
{
    struct fdb_blob blob;

    pms_device_t sensor = RT_NULL;
    struct pms_response resp;
    int count = 10, ret = 0;

    sensor = pms_create(UART_NAME);
    if (!sensor)
    {
        rt_kprintf("Init PMS5003ST sensor failed!\n");
        return;
    }

    pms_set_mode(sensor, PMS_MODE_NORMAL);
    pms_set_mode(sensor, PMS_MODE_PASSIVE);

    FDB_INFO("==================== TSDB append ====================\n");
    while (count--)
    {
        memset(&resp, 0, sizeof(resp));
        ret = pms_read(sensor, &resp, sizeof(resp), rt_tick_from_millisecond(3000));

        if (ret == sizeof(resp))
        {
            /* APPEND new TSL (time series log) */
            struct env_status status;

            status.pm25 = resp.PM2_5_atm;
            status.hcho = resp.hcho;
            status.temp = resp.temp;
            status.humi = resp.humi;

            fdb_tsl_append(&tsdb, fdb_blob_make(&blob, &status, sizeof(status)));
            FDB_INFO("append a new data: PM2.5 (%d), HCHO (%d), Temp (%d), Humi (%d)\n",
                      status.pm25, status.hcho, status.temp, status.humi);
        }

        rt_thread_mdelay(3000);
    }

    pms_delete(sensor);
    FDB_INFO("=====================================================\n");
}
MSH_CMD_EXPORT(read_sensor_data_to_tsdb, Storage sensor data in TSDB);

static bool query_cb(fdb_tsl_t tsl, void *arg)
{
    struct fdb_blob blob;
    struct env_status status;
    fdb_tsdb_t db = arg;

    fdb_blob_read((fdb_db_t) db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &status, sizeof(status))));
    FDB_INFO("queried a TSL: Time: %ld, PM2.5: %d, HCHO: %d, Temp: %d, Humi: %d\n",
              tsl->time, status.pm25, status.hcho, status.temp, status.humi);

    return false;
}

static void query_sensor_data_from_tsdb(void)
{
    FDB_INFO("==================== TSDB query =====================\n");
    /* Query all TSL in TSDB by iterator */
    fdb_tsl_iter(&tsdb, query_cb, &tsdb);
    FDB_INFO("=====================================================\n");
}
MSH_CMD_EXPORT(query_sensor_data_from_tsdb, Query sensor data from TSDB);

static void clean_sensor_data_in_tsdb(void)
{
    FDB_INFO("==================== TSDB clean =====================\n");
    fdb_tsl_clean(&tsdb);
    FDB_INFO("=====================================================\n");
}
MSH_CMD_EXPORT(clean_sensor_data_in_tsdb, Clean all sensor data in TSDB);

#endif /* FDB_USING_TSDB */
