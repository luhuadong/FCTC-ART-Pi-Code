/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-08-02     Rudy Lo      first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_common.h"
#include <wlan_mgnt.h>
#include <wlan_prot.h>
#include <wlan_cfg.h>
#include <ntp.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define WLAN_SSID               "FCTC_89"
#define WLAN_PASSWORD           "Lu15899962740"
#define NET_READY_TIME_OUT       (rt_tick_from_millisecond(15 * 1000))
#define NTP_AUTO_SYNC_PERIOD     (rt_tick_from_millisecond((1L*60L*60L) * 1000))

/* WLAN read 信号量 */
static rt_sem_t net_ready = RT_NULL;

/* NTP 时间同步线程 */
static void ntp_sync_thread_enrty(void *param)
{
    while (1)
    {
        rt_thread_delay(NTP_AUTO_SYNC_PERIOD);
        ntp_sync_to_rtc(NULL);
    }
}

/* 准备就绪回调函数 */
void wlan_ready_handler(int event, struct rt_wlan_buff *buff, void *parameter)
{
    /* 释放 ready 信号量 */
    rt_sem_release(net_ready);
}

/* 断开连接回调函数 */
void wlan_station_disconnect_handler(int event, struct rt_wlan_buff *buff, void *parameter)
{
    LOG_I("disconnect from the network!");
}

/* 打印 WLAN 信息 */
static void print_wlan_information(struct rt_wlan_info *info)
{
    LOG_D("+--------------------------+");
    LOG_D(" SSID : %-.32s", &info->ssid.val[0]);
    LOG_D(" MAC Addr: %02x:%02x:%02x:%02x:%02x:%02x",
            info->bssid[0], info->bssid[1], info->bssid[2],
            info->bssid[3], info->bssid[4], info->bssid[5]);
    LOG_D(" Channel: %d", info->channel);
    LOG_D(" DataRate: %dMbps", info->datarate / 1000000);
    LOG_D(" RSSI: %d", info->rssi);
    LOG_D("+--------------------------+");
}

extern int rt_hw_wlan_wait_init_done(rt_uint32_t time_ms);

int main(void)
{
    int result = RT_EOK;
    struct rt_wlan_info info;

    /* 等待 3s 以便 wifi 完成初始化 */
    rt_hw_wlan_wait_init_done(3000);

    /* 初始化信号量 */
    net_ready = rt_sem_create("net_ready", 0, RT_IPC_FLAG_PRIO);

    /* 设置为 STA 模式 */
    rt_wlan_set_mode(RT_WLAN_DEVICE_STA_NAME, RT_WLAN_STATION);

    /* 注册 wlan ready 回调函数 */
    rt_wlan_register_event_handler(RT_WLAN_EVT_READY, wlan_ready_handler, RT_NULL);
    /* 注册 wlan 断开回调函数 */
    rt_wlan_register_event_handler(RT_WLAN_EVT_STA_DISCONNECTED, wlan_station_disconnect_handler, RT_NULL);

    /* 热点连接 */
    LOG_D("start to connect ap ...");

    result = rt_wlan_connect(WLAN_SSID, WLAN_PASSWORD);
    if (result != RT_EOK) {
        LOG_E("The AP(%s) is connect failed!", WLAN_SSID);
    }

    /* 获取当前连接热点信息 */
    rt_memset(&info, 0, sizeof(struct rt_wlan_info));
    rt_wlan_get_info(&info);
    print_wlan_information(&info);

    /* 等待 Wi-Fi 连接成功 */
    if (RT_EOK == rt_sem_take(net_ready, NET_READY_TIME_OUT)) {
        LOG_D("networking ready!");
        /* 同步网络时间，使用默认NTP服务器 */
        ntp_sync_to_rtc(RT_NULL);
        /* 启动 NTP网络时间同步线程 */
        rt_thread_t thread = rt_thread_create("ntp_sync", ntp_sync_thread_enrty, RT_NULL, 1536, 26, 2);
        if (thread) rt_thread_startup(thread);
    }
    else {
        LOG_D("wait ip got timeout!");
    }

    /* 自动连接 */
    LOG_D("enable autoconnect ...");
    /* 使能 wlan 自动连接 */
    rt_wlan_config_autoreconnect(RT_TRUE);

    /* 回收资源 */
    rt_wlan_unregister_event_handler(RT_WLAN_EVT_READY);
    rt_sem_delete(net_ready);

    LOG_D("done!");
    return 0;
}

#include "stm32h7xx.h"
static int vtor_config(void)
{
    /* Vector Table Relocation in Internal QSPI_FLASH */
    SCB->VTOR = QSPI_BASE;
    return 0;
}
INIT_BOARD_EXPORT(vtor_config);

