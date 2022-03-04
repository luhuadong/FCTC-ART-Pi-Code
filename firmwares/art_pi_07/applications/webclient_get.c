/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-03-02     Rudy Lo      the first version
 */

#include <rtthread.h>
#include <webclient.h>

#define GET_HEADER_BUFSZ           1024
#define GET_RESP_BUFSZ             1024

#define HTTP_GET_URL               "http://www.rt-thread.com/service/rt-thread.txt"


/* send HTTP GET request by simplify request interface, it used to received shorter data */

static int webclient_get_comm(const char *uri)
{
    struct webclient_session* session = RT_NULL;
    unsigned char *buffer = RT_NULL;
    int index, ret = 0;
    int bytes_read, resp_status;
    int content_length = -1;
    char *header = RT_NULL;

    buffer = (unsigned char *) web_malloc(GET_RESP_BUFSZ);
    if (buffer == RT_NULL)
    {
        rt_kprintf("no memory for receive buffer.\n");
        ret = -RT_ENOMEM;
        goto __exit;
    }

    /* 创建 webclient 会话，设置响应头部大小 */
    session = webclient_session_create(GET_HEADER_BUFSZ);
    if (session == RT_NULL)
    {
        ret = -RT_ENOMEM;
        goto __exit;
    }

    /* 拼接自定义头部数据 */
    webclient_request_header_add(&header, "Accept: application/json\r\n");
    webclient_request_header_add(&header, "User-Agent: RT-Thread HTTP Agent\r\n");
    webclient_request_header_add(&header, "Connection: close\r\n");

    /* 发送 GET 请求 */
    if ((resp_status = webclient_get(session, uri)) != 200)
    {
        rt_kprintf("webclient GET request failed, response(%d) error.\n", resp_status);
        ret = -RT_ERROR;
        goto __exit;
    }

    rt_kprintf("webclient get response data: \n");

    content_length = webclient_content_length_get(session);
    if (content_length < 0)
    {
        /* 获取失败 */
        rt_kprintf("webclient GET request type is chunked.\n");
        goto __exit;
    }

    int content_pos = 0;

    do
    {
        bytes_read = webclient_read(session, (void *)buffer,
                                    content_length - content_pos > GET_RESP_BUFSZ ?
                                    GET_RESP_BUFSZ : content_length - content_pos);
        if (bytes_read <= 0) {
            /* 读取完毕 */
            break;
        }

        /* 打印响应数据 */
        for (index = 0; index < bytes_read; index++) {
            rt_kprintf("%c", buffer[index]);
        }
        content_pos += bytes_read;

    } while (content_pos < content_length);

    rt_kprintf("\n");

__exit:
    if (session)
        webclient_close(session);

    if (buffer)
        web_free(buffer);

    return ret;
}

static int webclient_get_data(const char *uri)
{
    char *header = RT_NULL;
    char *response = RT_NULL;
    size_t resp_len = 0;

    /* 拼接自定义头部数据 */
    webclient_request_header_add(&header, "User-Agent: GetIoT HTTP Agent\r\n");

    /* 发送请求，第3个参数为NULL表示GET请求 */
    if (webclient_request(uri, header, RT_NULL, 0, (void **)&response, &resp_len) < 0) {
        rt_kprintf("webclient GET request response data error.\n");
        web_free(header);
        return -RT_ERROR;
    }

    /* 打印响应数据 */
    rt_kprintf("webclient get response data: \n");
    for (int index = 0; index < resp_len; index++) {
        rt_kprintf("%c", response[index]);
    }
    rt_kprintf("\n");

    /* 释放内存 */
    if (header) {
        web_free(header);
    }
    if (response) {
        web_free(response);
    }

    return 0;
}

int http_get_test(int argc, char **argv)
{
    char *uri = RT_NULL;

    /* 如果没有输入URI，则使用默认的测试地址 */
    if (argc == 1) {
        uri = web_strdup(HTTP_GET_URL);
    }
    else if (argc == 2) {
        uri = web_strdup(argv[1]);
    }
    else {
        rt_kprintf("Usage: %s [uri]\n", argv[0]);
        return -RT_ERROR;
    }

    /* 发送GET请求并获取响应数据 */
    webclient_get_data(uri);
    /* 释放资源 */
    web_free(uri);

    return RT_EOK;
}
MSH_CMD_EXPORT(http_get_test, webclient get request test);
