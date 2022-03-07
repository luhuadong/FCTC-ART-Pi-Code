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
#include "dev_sign_api.h"
#include "mqtt_api.h"
#include "mqtt_wrapper.h"

/* 设备三元组信息 */
static char PRODUCT_KEY[IOTX_PRODUCT_KEY_LEN + 1] = {0};
static char DEVICE_NAME[IOTX_DEVICE_NAME_LEN + 1] = {0};
static char DEVICE_SECRET[IOTX_DEVICE_SECRET_LEN + 1] = {0};

#define TOPIC_FMT        "/%s/%s/user/get"
//#define TOPIC_FMT        "/%s/%s/hello"
#define TEST_PAYLOAD     "{\"message\":\"hello!\"}"

#define EXAMPLE_TRACE(fmt, ...)  \
    do { \
        HAL_Printf("%s|%03d :: ", __func__, __LINE__); \
        HAL_Printf(fmt, ##__VA_ARGS__); \
        HAL_Printf("%s", "\r\n"); \
    } while(0)

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

static int example_publish(void *handle)
{
    int             res = 0;
    //const char     *fmt = "/%s/%s/user/get";
    char           *topic = NULL;
    int             topic_len = 0;
    //char           *payload = "{\"message\":\"hello!\"}";

    topic_len = strlen(TOPIC_FMT) + strlen(PRODUCT_KEY) + strlen(DEVICE_NAME) + 1;
    topic = HAL_Malloc(topic_len);
    if (topic == NULL) {
        EXAMPLE_TRACE("memory not enough");
        return -1;
    }
    memset(topic, 0, topic_len);
    HAL_Snprintf(topic, topic_len, TOPIC_FMT, PRODUCT_KEY, DEVICE_NAME);

    res = IOT_MQTT_Publish_Simple(0, topic, IOTX_MQTT_QOS0, TEST_PAYLOAD, strlen(TEST_PAYLOAD));
    if (res < 0) {
        EXAMPLE_TRACE("publish failed, res = %d", res);
        HAL_Free(topic);
        return -1;
    }

    HAL_Free(topic);
    return 0;
}

static void example_event_handle(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    EXAMPLE_TRACE("msg->event_type : %d", msg->event_type);
}

static int mqtt_example_main(int argc, char **argv)
{
    void                   *pclient = NULL;
    int                     res = 0;
    int                     loop_cnt = 0;
    iotx_mqtt_param_t       mqtt_params;

    HAL_GetProductKey(PRODUCT_KEY);
    HAL_GetDeviceName(DEVICE_NAME);
    HAL_GetDeviceSecret(DEVICE_SECRET);

    EXAMPLE_TRACE("mqtt example");

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
     *  设置 MQTT 连接端口号
     *  TCP/TLS 端口号通常为 443、1883 或 80 等等，在
     *  阿里云 IoT 平台 MQTT 默认端口是 1883，MQTTS 默认端口为 443
     */
    //mqtt_params.port = 1883;

    /*
     * 设置 MQTT 请求超时（发送 MQTT 消息后等待 ACK 回应的时间）
     * 默认值为 2000ms
     */
    //mqtt_params.request_timeout_ms = 2000;

    /*
     * MQTT clean session flag
     *
     * If CleanSession is set to 0, the Server MUST resume communications with the Client based on state from
     * the current Session (as identified by the Client identifier).
     *
     * If CleanSession is set to 1, the Client and Server MUST discard any previous Session and Start a new one.
     *
     * default value is 0.
     */
    //mqtt_params.clean_session = 0;

    /*
     * MQTT keepAlive interval
     *
     * KeepAlive is the maximum time interval that is permitted to elapse between the point at which
     * the Client finishes transmitting one Control Packet and the point it starts sending the next.
     *
     * default value is 60000.
     */
    //mqtt_params.keepalive_interval_ms = 60000;

    /*
     * MQTT write buffer size
     *
     * Write buffer is allocated to place upstream MQTT messages, MQTT client will be limitted
     * to send packet no longer than this to Cloud
     *
     * default value is 1024.
     *
     */
    //mqtt_params.write_buf_size = 1024;

    /*
     * 设置 MQTT 读缓冲区大小
     *
     * Write buffer is allocated to place downstream MQTT messages, MQTT client will be limitted
     * to recv packet no longer than this from Cloud
     *
     * default value is 1024.
     *
     */
    //mqtt_params.read_buf_size = 1024;

    /*
     * MQTT event callback function
     *
     * Event callback function will be called by SDK when it want to notify user what is happening inside itself
     *
     * default value is NULL, which means PUB/SUB event won't be exposed.
     *
     */
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
        if (0 == loop_cnt % 200) {
            example_publish(pclient);
        }

        IOT_MQTT_Yield(pclient, 200);

        loop_cnt += 1;
    }

    return 0;
}
MSH_CMD_EXPORT(mqtt_example_main, Run mqtt example)
