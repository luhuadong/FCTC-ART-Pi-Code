/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-10-02     Rudy Lo      the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <webnet.h>
#include <wn_module.h>
#include <cJSON.h>
#include "pmsxx.h"
#include "mb.h"
#include "mb_m.h"

#define SENSOR_UART_NAME   "uart6"

#define SLAVE_ADDR         0x01         /* 从设备地址 */
#define PORT_NUM           5            /* 串口号（UART5） */
#define PORT_BAUDRATE      9600         /* 波特率 */
#define PORT_PARITY        MB_PAR_NONE  /* 无奇偶校验 */
#define MB_POLL_CYCLE_MS   500          /* Modbus 主站轮询周期 */

#define cgi_head()                       \
    ;                                    \
    const char *mimetype;                \
    struct webnet_request *request;      \
    static char *body = NULL;            \
    request = session->request;          \
    mimetype = mime_get_type(".html");   \
    session->request->result_code = 200; \
    webnet_session_set_header(session, mimetype, 200, "Ok", -1);

static int turn_off_coil(rt_uint16_t num);
static int turn_on_coil(rt_uint16_t num);

/* 保存最新的传感器数据 */
static struct local_sensor_data {
    rt_int16_t temp;
    rt_int16_t humi;
    rt_int16_t pm25;
    rt_int16_t hcho;
} sensor_data = {0, 0, 0, 0};

/* Web API: /cgi-bin/sensor_data */
static void cgi_sensor_data(struct webnet_session *session)
{
    cgi_head();
    cJSON *root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "temp", cJSON_CreateNumber(sensor_data.temp));
    cJSON_AddItemToObject(root, "humi", cJSON_CreateNumber(sensor_data.humi));
    cJSON_AddItemToObject(root, "pm25", cJSON_CreateNumber(sensor_data.pm25));
    cJSON_AddItemToObject(root, "hcho", cJSON_CreateNumber(sensor_data.hcho));

    char *json_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    webnet_session_printf(session, json_data);
    rt_free(json_data);
}

/* Web API: /cgi-bin/relay_control */
static void cgi_relay_control(struct webnet_session *session)
{
    cgi_head();
    if (session->request->query)
    {
        cJSON *root = cJSON_Parse(session->request->query);
        if (root)
        {
            cJSON *id = cJSON_GetObjectItem(root, "id");
            cJSON *sw = cJSON_GetObjectItem(root, "sw");
            if (0 == strcmp(sw->valuestring, "On")) {
                turn_on_coil(id->valueint);
            }
            else if (0 == strcmp(sw->valuestring, "Off")) {
                turn_off_coil(id->valueint);
            }
            cJSON_Delete(root);
        }
    }
    rt_kprintf("%s\n", session->request->query);
    webnet_session_printf(session, "relay control");
}

static void basic_init(void)
{
    /* 注册 CGI 事件处理函数 */
    webnet_cgi_register("sensor_data", cgi_sensor_data);     /* GET */
    webnet_cgi_register("relay_control", cgi_relay_control); /* POST */
}

static void pms5003_thread(void *parameter)
{
    int ret = 0;
    struct pms_response resp;

    pms_device_t sensor = pms_create(SENSOR_UART_NAME);

    if (!sensor)
    {
        rt_kprintf("Init PMS5003ST sensor failed!\n");
        return -1;
    }

    pms_set_mode(sensor, PMS_MODE_NORMAL);
    pms_set_mode(sensor, PMS_MODE_PASSIVE);

    while (1)
    {
        rt_memset(&resp, 0, sizeof(resp));

        //rt_kprintf("\nRequest...\n");
        ret = pms_read(sensor, &resp, sizeof(resp), rt_tick_from_millisecond(3000));

        if (ret == sizeof(resp))
        {
            //pms_show_response(&resp);
            sensor_data.temp = resp.temp;
            sensor_data.humi = resp.humi;
            sensor_data.pm25 = resp.PM2_5_atm;
            sensor_data.hcho = resp.hcho;
        }
        else
        {
            rt_kprintf("Read error\n");
        }
        rt_thread_mdelay(3000);
    }

    pms_delete(sensor);
    return RT_EOK;
}

static void sensor_init(void)
{
    rt_thread_t tid;

    tid = rt_thread_create("pms5003", pms5003_thread, RT_NULL, 1024, 8, 0);
    if (tid)
        rt_thread_startup(tid);
}

static int turn_on_coil(rt_uint16_t num)
{
    eMBMasterReqErrCode error_code = MB_MRE_NO_ERR;

    error_code = eMBMasterReqWriteCoil(SLAVE_ADDR, num, 0xFF00, rt_tick_from_millisecond(3000));

    if (error_code != MB_MRE_NO_ERR)
    {
        rt_kprintf("Error code: %d\n", error_code);
        return -RT_ERROR;
    }
    return RT_EOK;
}

static int turn_off_coil(rt_uint16_t num)
{
    eMBMasterReqErrCode error_code = MB_MRE_NO_ERR;

    error_code = eMBMasterReqWriteCoil(SLAVE_ADDR, num, 0x0000, rt_tick_from_millisecond(3000));

    if (error_code != MB_MRE_NO_ERR)
    {
        rt_kprintf("Error code: %d\n", error_code);
        return -RT_ERROR;
    }
    return RT_EOK;
}

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

static void modbus_init(void)
{
    rt_thread_t tid = RT_NULL;
    tid = rt_thread_create("md_poll", mb_master_poll, RT_NULL, 512, 10, 10);

    if (tid)
        rt_thread_startup(tid);
}

static void webnet_server(void)
{
    sensor_init();  /* 初始化传感器 */
    modbus_init();  /* 初始化Modbus */
    basic_init();   /* 注册 CGI 事件 */
    webnet_init();  /* 启动 Web 服务器 */
}
MSH_CMD_EXPORT(webnet_server, wenbet test);
