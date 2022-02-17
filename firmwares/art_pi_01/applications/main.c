/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-09-02     RT-Thread    first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_common.h"

#define LED_B_PIN      GET_PIN(I, 8)
#define LED_R_PIN      GET_PIN(C, 15)
#define USER_KEY1_PIN  GET_PIN(H, 4)

void key_pressed(void *args)
{
    static rt_uint8_t flag = 0;
    rt_kprintf("key pressed! %d\n", flag);

    switch(flag) {
    case 0: {
            rt_pin_write(LED_B_PIN, PIN_LOW);
            rt_pin_write(LED_R_PIN, PIN_HIGH);
            flag++;
            break;
        }
    case 1: {
            rt_pin_write(LED_B_PIN, PIN_HIGH);
            rt_pin_write(LED_R_PIN, PIN_LOW);
            flag++;
            break;
        }
    case 2: {
            rt_pin_write(LED_B_PIN, PIN_LOW);
            rt_pin_write(LED_R_PIN, PIN_LOW);
            flag++;
            break;
        }
    case 3: {
            rt_pin_write(LED_B_PIN, PIN_HIGH);
            rt_pin_write(LED_R_PIN, PIN_HIGH);
            flag = 0;
            break;
        }
    default: break;
    }
}

int main(void)
{
    /* LED 引脚为输出模式 */
    rt_pin_mode(LED_B_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(LED_R_PIN, PIN_MODE_OUTPUT);

    /* 初始状态 LED 不亮 */
    rt_pin_write(LED_B_PIN, PIN_HIGH);
    rt_pin_write(LED_R_PIN, PIN_HIGH);

    /* 按键 KEY1 引脚为输入模式 */
    rt_pin_mode(USER_KEY1_PIN, PIN_MODE_INPUT_PULLUP);
    /* 绑定中断，下降沿模式，回调函数名为 key_pressed */
    rt_pin_attach_irq(USER_KEY1_PIN, PIN_IRQ_MODE_FALLING, key_pressed, RT_NULL);
    /* 使能中断 */
    rt_pin_irq_enable(USER_KEY1_PIN, PIN_IRQ_ENABLE);

    return RT_EOK;
}

#include "stm32h7xx.h"
static int vtor_config(void)
{
    /* Vector Table Relocation in Internal QSPI_FLASH */
    SCB->VTOR = QSPI_BASE;
    return 0;
}
INIT_BOARD_EXPORT(vtor_config);


