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

#define GET_HEADER_BUFSZ           4096
#define GET_RESP_BUFSZ             1024

#define GET_LOCAL_URI              "https://api.github.com/repos/RT-Thread/rt-thread"

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
        rt_kprintf("webclient GET request type is chunked.\n");
        do
        {
            bytes_read = webclient_read(session, (void *)buffer, GET_RESP_BUFSZ);
            if (bytes_read <= 0)
            {
                break;
            }

            for (index = 0; index < bytes_read; index++)
            {
                rt_kprintf("%c", buffer[index]);
            }
        } while (1);

        rt_kprintf("\n");
    }
    else
    {
        int content_pos = 0;

        do
        {
            bytes_read = webclient_read(session, (void *)buffer,
                    content_length - content_pos > GET_RESP_BUFSZ ?
                            GET_RESP_BUFSZ : content_length - content_pos);
            if (bytes_read <= 0)
            {
                break;
            }

            for (index = 0; index < bytes_read; index++)
            {
                rt_kprintf("%c", buffer[index]);
            }

            content_pos += bytes_read;
            rt_kprintf("\n>>\n");
        } while (content_pos < content_length);

        rt_kprintf("\n");
    }

__exit:
    if (session)
        webclient_close(session);

    if (buffer)
        web_free(buffer);

    return ret;
}

int https_get_github(int argc, char **argv)
{
    char *uri = RT_NULL;

    if (argc == 1) {
        uri = web_strdup(GET_LOCAL_URI);
    }
    else if (argc == 2) {
        uri = web_strdup(argv[1]);
    }
    else {
        rt_kprintf("Usage: %s <uri>\n", argv[0]);
        return -RT_ERROR;
    }

    webclient_get_comm(uri);
    web_free(uri);

    return RT_EOK;
}
MSH_CMD_EXPORT(https_get_github, https get request test);
