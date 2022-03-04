/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-03-02     Rudy Lo      the first version
 */

#include <string.h>
#include <rtthread.h>
#include <webclient.h>

#define HTTP_POST_URL             "http://www.rt-thread.com/service/echo"

const char *post_data = "FCTC is the abbreviation of From Chip To Cloud.";

/* send HTTP POST request by simplify request interface, it used to received shorter data */
static int webclient_post_data(const char *uri, const char *post_data, size_t data_len)
{
    char *response = RT_NULL;
    char *header = RT_NULL;
    size_t resp_len = 0;

    /* 拼接自定义头部数据 */
    webclient_request_header_add(&header, "Content-Length: %d\r\n", strlen(post_data));
    webclient_request_header_add(&header, "Content-Type: application/octet-stream\r\n");

    /* 发送POST请求 */
    if (webclient_request(uri, header, post_data, data_len, (void **)&response, &resp_len) < 0)
    {
        rt_kprintf("webclient send post request failed.");
        web_free(header);
        return -RT_ERROR;
    }

    /* 打印响应数据 */
    rt_kprintf("webclient post response data: \n");
    for (int index = 0; index < resp_len; index++)
    {
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

int http_post_test(int argc, char **argv)
{
    char *uri = RT_NULL;

    /* 如果没有输入URI，则使用默认的测试地址 */
    if (argc == 1) {
        uri = web_strdup(HTTP_POST_URL);
    }
    else if (argc == 2) {
        uri = web_strdup(argv[1]);
    }
    else {
        rt_kprintf("Usage: %s [uri]\n", argv[0]);
        return -RT_ERROR;
    }

    /* 发送POST请求并获取响应数据 */
    webclient_post_data(uri, (void *)post_data, rt_strlen(post_data));

    /* 释放资源 */
    web_free(uri);

    return RT_EOK;
}
MSH_CMD_EXPORT(http_post_test, webclient post request test);
