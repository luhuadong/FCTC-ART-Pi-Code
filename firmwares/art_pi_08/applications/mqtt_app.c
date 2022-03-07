/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-03-07     Rudy Lo      the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "dev_sign_api.h"
#include "mqtt_api.h"
#include "mqtt_wrapper.h"
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

/* 保存最新的传感器数据 */
static struct local_sensor_data {
    rt_int16_t temp;
    rt_int16_t humi;
    rt_int16_t pm25;
    rt_int16_t hcho;
} sensor_data = {0, 0, 0, 0};

/* 设备三元组信息 */
static char PRODUCT_KEY[IOTX_PRODUCT_KEY_LEN + 1] = {0};
static char DEVICE_NAME[IOTX_DEVICE_NAME_LEN + 1] = {0};
static char DEVICE_SECRET[IOTX_DEVICE_SECRET_LEN + 1] = {0};

static void *pclient = NULL;

#define TOPIC_FMT              "/%s/%s/user/get"
#define TOPIC_PROPERTY_POST    "/sys/%s/%s/thing/event/property/post"

#define TEST_PAYLOAD     "{\"message\":\"hello!\"}"
#define PROPERTY_PAYLOAD "{\"id\":\"123\",\"version\":\"1.0\",\"sys\":{\"ack\":0},\
                           \"params\":{\"Temp\":{\"value\":\"99.9\"}, \
                           \"method\":\"thing.event.property.post\"}"

#define EXAMPLE_TRACE(fmt, ...)  \
    do { \
        HAL_Printf("%s|%03d :: ", __func__, __LINE__); \
        HAL_Printf(fmt, ##__VA_ARGS__); \
        HAL_Printf("%s", "\r\n"); \
    } while(0)

static int turn_off_coil(rt_uint16_t num);
static int turn_on_coil(rt_uint16_t num);

static void example_message_arrive(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    iotx_mqtt_topic_info_t     *topic_info = (iotx_mqtt_topic_info_pt) msg->msg;

    switch (msg->event_type) {
        case IOTX_MQTT_EVENT_PUBLISH_RECEIVED:
            /* print topic name and topic message */
            EXAMPLE_TRACE("Message Arrived:");
            EXAMPLE_TRACE("Topic  : %.*s", topic_info->topic_len, topic_info->ptopic);
            EXAMPLE_TRACE("Payload: %.*s", topic_info->payload_len, topic_info->payload);
            EXAMPLE_TRACE("\n");
            break;
        default:
            break;
    }
}

static int example_subscribe(void *handle)
{
    int res = 0;
    //const char *fmt = "/%s/%s/user/get";
    char *topic = NULL;
    int topic_len = 0;

    topic_len = strlen(TOPIC_FMT) + strlen(PRODUCT_KEY) + strlen(DEVICE_NAME) + 1;
    topic = HAL_Malloc(topic_len);
    if (topic == NULL) {
        EXAMPLE_TRACE("memory not enough");
        return -1;
    }
    memset(topic, 0, topic_len);
    HAL_Snprintf(topic, topic_len, TOPIC_FMT, PRODUCT_KEY, DEVICE_NAME);

    res = IOT_MQTT_Subscribe(handle, topic, IOTX_MQTT_QOS0, example_message_arrive, NULL);
    if (res < 0) {
        EXAMPLE_TRACE("subscribe failed");
        HAL_Free(topic);
        return -1;
    }

    HAL_Free(topic);
    return 0;
}

static int publish_properties(void *handle, int value)
{
    int             res = 0;
    char           *topic = NULL;
    int             topic_len = 0;
    char           *payload = NULL;

    topic_len = strlen(TOPIC_PROPERTY_POST) + strlen(PRODUCT_KEY) + strlen(DEVICE_NAME) + 80;
    topic = HAL_Malloc(topic_len);
    if (topic == NULL) {
        EXAMPLE_TRACE("memory not enough");
        return -1;
    }
    memset(topic, 0, topic_len);
    HAL_Snprintf(topic, topic_len, TOPIC_PROPERTY_POST, PRODUCT_KEY, DEVICE_NAME);

    payload = HAL_Malloc(1024);
    if (payload == NULL) {
        EXAMPLE_TRACE("memory not enough");
        return -1;
    }

    HAL_Snprintf(payload, 1024, PROPERTY_PAYLOAD, value/10, value%10);

    res = IOT_MQTT_Publish_Simple(0, topic, IOTX_MQTT_QOS0, PROPERTY_PAYLOAD, strlen(PROPERTY_PAYLOAD));
    if (res < 0) {
        EXAMPLE_TRACE("publish failed, res = %d", res);
        HAL_Free(topic);
        HAL_Free(payload);
        return -1;
    }

    HAL_Free(topic);
    HAL_Free(payload);
    return 0;
}

static void example_event_handle(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    EXAMPLE_TRACE("msg->event_type : %d", msg->event_type);
}

static int mqtt_init(void)
{
    int                     res = 0;
    int                     loop_cnt = 0;
    iotx_mqtt_param_t       mqtt_params;

    HAL_GetProductKey(PRODUCT_KEY);
    HAL_GetDeviceName(DEVICE_NAME);
    HAL_GetDeviceSecret(DEVICE_SECRET);

    EXAMPLE_TRACE("mqtt application");

    /*
     * 初始化 MQTT 参数
     * 如果没有设置 mqtt_params 结构体成员的值，SDK 将使用默认的值
     **/
    memset(&mqtt_params, 0x0, sizeof(mqtt_params));

    /*
     *  设置 MQTT 连接服务器的主机地址
     *  默认值为 ${productKey}.iot-as-mqtt.cn-shanghai.aliyuncs.com
     */
    //mqtt_params.host = "something.iot-as-mqtt.cn-shanghai.aliyuncs.com";

    /*
     *  设置 MQTT 连接端口号，TCP/TLS 端口号通常为 443、1883 或 80 等
     *  阿里云 IoT 平台 MQTT 默认端口是 1883，MQTTS 默认端口为 443
     */
    //mqtt_params.port = 1883;

    /* 设置 MQTT 请求超时（发送 MQTT 消息后等待 ACK 回应的时间），默认值为 2000ms */
    //mqtt_params.request_timeout_ms = 2000;

    /* 设置 MQTT 清除会话标志，默认值为0 */
    //mqtt_params.clean_session = 0;

    /* 设置 MQTT keepAlive 间隔，默认值为60秒 */
    //mqtt_params.keepalive_interval_ms = 60000;

    /* 设置 MQTT 写缓冲区大小，默认值为1024 */
    //mqtt_params.write_buf_size = 1024;

    /* 设置 MQTT 读缓冲区大小，默认值为1024 */
    //mqtt_params.read_buf_size = 1024;

    /* 绑定 MQTT 事件回调函数 */
    mqtt_params.handle_event.h_fp = example_event_handle;

    pclient = IOT_MQTT_Construct(&mqtt_params);
    if (NULL == pclient) {
        EXAMPLE_TRACE("MQTT construct failed");
        return -1;
    }

    res = example_subscribe(pclient);
    if (res < 0) {
        IOT_MQTT_Destroy(&pclient);
        EXAMPLE_TRACE("MQTT subscribe failed");
        return -1;
    }

    while (1) {
        IOT_MQTT_Yield(pclient, 200);
    }

    return 0;
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
        ret = pms_read(sensor, &resp, sizeof(resp), rt_tick_from_millisecond(3000));

        if (ret == sizeof(resp))
        {
            sensor_data.temp = resp.temp;
            sensor_data.humi = resp.humi;
            sensor_data.pm25 = resp.PM2_5_atm;
            sensor_data.hcho = resp.hcho;
            if (pclient)
                publish_properties(pclient, sensor_data.temp);
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

    tid = rt_thread_create("pms5003", pms5003_thread, RT_NULL, 4096, 8, 0);
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

static void run_mqtt_app(void)
{
    rt_kprintf(">> 1\n");

    rt_kprintf(">> 2\n");
    sensor_init();  /* 初始化传感器 */
    rt_kprintf(">> 3\n");
    //modbus_init();  /* 初始化Modbus */
    rt_kprintf(">> 4\n");
    mqtt_init();    /* 启动 Web 服务器 */
}
MSH_CMD_EXPORT(run_mqtt_app, Run mqtt application)
