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
#include <sys/socket.h> /* 使用BSD socket，需要包含sockets.h头文件 */
#include <netdb.h>
#include <string.h>
#include <finsh.h>

#define BUFSZ   1024

/* 发送用到的数据 */
const char send_data[] = "This is UDP Client from RT-Thread.";

void udpclient(int argc, char **argv)
{
    int sock, port, count, bytes_received;
    char recv_data[BUFSZ];
    struct hostent *host;
    struct sockaddr_in server_addr, their_addr;
    socklen_t len;
    const char *url;

    if (argc < 3)
    {
        rt_kprintf("Usage: udpclient <URL> <PORT> [COUNT = 10]\n");
        return ;
    }

    url = argv[1];
    port = strtoul(argv[2], 0, 10);

    if (argc > 3)
        count = strtoul(argv[3], 0, 10);
    else
        count = 10;

    /* 通过函数入口参数url获得host地址（如果是域名，会做域名解析） */
    host = (struct hostent *) gethostbyname(url);

    /* 创建一个socket，类型是SOCK_DGRAM，UDP类型 */
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        rt_kprintf("Socket error\n");
        return;
    }

    /* 初始化预连接的服务端地址 */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    rt_memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));

    len = sizeof(their_addr);

    /* 总计发送count次数据 */
    while (count--)
    {
        /* 发送数据到服务远端 */
        sendto(sock, send_data, strlen(send_data), 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));

        /* 等待数据 */
        bytes_received = recvfrom(sock, recv_data, BUFSZ - 1, 0, (struct sockaddr *)&their_addr, &len);

        if (bytes_received > 0) {
            /* 有接收到数据，把末端清零 */
            recv_data[bytes_received] = '\0';
            /* 在控制终端显示收到的数据 */
            rt_kprintf("Recv: %s\n", recv_data);
        }
        /* 线程休眠一段时间 */
        rt_thread_mdelay(1000);
    }

    sendto(sock, "exit", strlen("exit"), 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));

    /* 关闭这个socket */
    closesocket(sock);
}

MSH_CMD_EXPORT(udpclient, a udp client sample);
