/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-03-02     Rudy Lo      the first version
 */

/*
 * 程序清单：tcp 客户端
 *
 * 这是一个 tcp 客户端的例程
 * 导出 tcpclient 命令到控制终端
 * 命令调用格式：tcpclient URL PORT
 * URL：服务器地址 PORT:：端口号
 * 程序功能：接收并显示从服务端发送过来的信息，接收到开头是 'q' 或 'Q' 的信息退出程序
 */
#include <rtthread.h>
#include <sys/socket.h> /* 使用BSD socket，需要包含socket.h头文件 */
#include <netdb.h>
#include <string.h>
#include <finsh.h>

#define BUFSZ   1024

static const char send_data[] = "This is TCP Client from RT-Thread."; /* 发送用到的数据 */

void tcpclient(int argc, char **argv)
{
    int ret, cnt = 10;
    char recv_data[BUFSZ];
    struct hostent *host;
    int sock, bytes_received;
    struct sockaddr_in server_addr;
    const char *url;
    int port;

    if (argc < 3)
    {
        rt_kprintf("Usage: tcpclient <URL> <PORT>\n");
        return;
    }

    url = argv[1];
    port = strtoul(argv[2], 0, 10);

    /* 通过函数入口参数url获得host地址（如果是域名，会做域名解析） */
    host = gethostbyname(url);

    /* 创建一个socket，类型是SOCKET_STREAM，TCP类型 */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        /* 创建socket失败 */
        rt_kprintf("Socket error\n");
        return;
    }

    /* 初始化预连接的服务端地址 */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = *((struct in_addr *) host->h_addr);
    rt_memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));

    /* 连接到服务端 */
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
    {
        /* 连接失败 */
        rt_kprintf("Connect fail!\n");
        closesocket(sock);
        return;
    }

    /* 连接成功 */
    rt_kprintf("Connect successful\n");

    while (cnt--)
    {
        ret = send(sock, send_data, strlen(send_data), 0);
        if (ret < 0)
        {
            /* 接收失败，关闭这个连接 */
            closesocket(sock);
            rt_kprintf("send error, close the socket.\n");
            break;
        }

        /* 从sock连接中接收最大BUFSZ - 1字节数据 */
        bytes_received = recv(sock, recv_data, BUFSZ - 1, 0);
        if (bytes_received < 0)
        {
            /* 接收失败，关闭这个连接 */
            closesocket(sock);
            rt_kprintf("received error,close the socket.\n");
            break;
        }
        else if (bytes_received == 0)
        {
            /* 默认 recv 为阻塞模式，此时收到0认为连接出错，关闭这个连接 */
            closesocket(sock);
            rt_kprintf("received error,close the socket.\n");
            break;
        }

        /* 有接收到数据，把末端清零 */
        recv_data[bytes_received] = '\0';
        /* 在控制终端显示收到的数据 */
        rt_kprintf("Recv: %s\n", recv_data);

        rt_thread_mdelay(1000);
    }

    send(sock, "exit", strlen("exit"), 0);
    closesocket(sock);
    rt_kprintf("exit\n");
}

MSH_CMD_EXPORT(tcpclient, a tcp client sample);
