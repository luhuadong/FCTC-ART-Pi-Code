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
#include <cJSON.h>

#define HTTP_GET_URL               "https://api.github.com/repos/RT-Thread/rt-thread"

static void parse_json_data(const char *data, const size_t len)
{
    cJSON *obj = RT_NULL;

    obj = cJSON_ParseWithLength(data, len);

    /* 从JSON对象中解析3组数据 */
    int subscribers_count = cJSON_GetNumberValue(cJSON_GetObjectItem(obj, "subscribers_count"));
    int forks             = cJSON_GetNumberValue(cJSON_GetObjectItem(obj, "forks"));
    int stargazers_count  = cJSON_GetNumberValue(cJSON_GetObjectItem(obj, "stargazers_count"));

    rt_kprintf("Watchers: %d\n", subscribers_count);
    rt_kprintf("Forks:    %d\n", forks);
    rt_kprintf("Stars:    %d\n", stargazers_count);
    
    cJSON_free(obj);
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

#if 0
    /* 打印响应数据 */
    rt_kprintf("webclient get response data: \n");
    for (int index = 0; index < resp_len; index++) {
        rt_kprintf("%c", response[index]);
    }
    rt_kprintf("\n");
#endif

    /* 解析并显示JSON数据 */
    parse_json_data(response, resp_len);

    /* 释放内存 */
    if (header) {
        web_free(header);
    }
    if (response) {
        web_free(response);
    }

    return 0;
}

int https_get_github(int argc, char **argv)
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
MSH_CMD_EXPORT(https_get_github, https get request test);
