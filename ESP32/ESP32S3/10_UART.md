# UART

## 分类

PPT 先把常见串行接口放在一起比较：

| 接口 | 引脚 | 双工 | 时钟 | 典型拓扑 |
|---|---|---|---|---|
| UART | TX、RX | 全双工 | 异步 | 一对一 |
| I2C | SCL、SDA | 半双工 | 同步 | 一对多 |
| SPI | SCLK、MOSI、MISO、CS | 全双工 | 同步 | 一对多 |

串行 / 并行是按“同时传几位”分；单工 / 半双工 / 全双工按传输方向分；同步 / 异步则看是否共享时钟。

## 连接

UART 是异步全双工通信。两设备最简单的连接是 **TX → RX、RX ← TX、GND ↔ GND**；如果电平标准不一致，要加电平转换。

![[assets/ppt_100.png|900]]

*PPT：UART TX/RX 交叉连接。*

课程还对 TTL、RS232、RS485 的“1/0 电压定义”做了区分。本开发板 UART GPIO 是 3.3 V TTL 逻辑，不能直接当 RS232 电平使用。

## 时序

空闲为高，起始位固定低，数据位低位先行，停止位固定高，可选奇偶校验。

![[assets/ppt_102.png|900]]

*PPT：波特率、起始位、停止位、数据位、校验位。*

![[assets/ppt_103.png|900]]

*PPT：9600 baud 的具体数据帧分析。*

## 外设

ESP32-S3 有 UART0/1/2。UART 硬件把内存中的字节自动变成 TX 时序，也能把 RX 波形自动还原为字节放进接收缓冲区。

![[assets/ppt_105.png|900]]

*PPT：UART 内部架构。*

![[assets/ppt_106.png|900]]

*PPT：UART 控制器、缓冲区、GPIO Matrix 及配置 API。*

视频简单回显工程：

**源码：`v5.1.2/15_Uart/components/MYUART/myuart.c`**

```c
#include "myuart.h"
#include "driver/uart.h"
#include "driver/gpio.h"

void uart_init(void)
{
    uart_config_t uart_structure = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .parity = UART_PARITY_DISABLE,
        .rx_flow_ctrl_thresh = 100,
        .source_clk = UART_SCLK_DEFAULT,
        .stop_bits = UART_STOP_BITS_1,
    };
    uart_param_config(UART_NUM_1, &uart_structure);

    uart_set_pin(UART_NUM_1, GPIO_NUM_17, GPIO_NUM_18, -1, -1);

    uart_driver_install(UART_NUM_1, 1024, 1024, 0, NULL,0);
}
```

**源码：`v5.1.2/15_Uart/main/main.c`**

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "myuart.h"

void app_main(void)
{
    int length = 0 ;
    uint8_t receive_data[1024] = {0};
    uart_init();
    while (1)
    {
        length = uart_read_bytes(UART_NUM_1, receive_data, 1024, 100);
        if(length > 0 )
        {
            uart_write_bytes(UART_NUM_1, receive_data, length);
            uart_flush(UART_NUM_1);
        }
        vTaskDelay(5);
    }
}
```

流程是：`uart_param_config()` → `uart_set_pin()` → `uart_driver_install()`；运行时 `uart_read_bytes()` 读缓冲区，`uart_write_bytes()` 写发送缓冲区。

## 数据格式

PPT 区分 HEX 与文本：HEX 显示原始字节，文本是对原始字节按字符编码解释。例如 `0x41` 在 ASCII 文本模式显示为 `A`。

![[assets/ppt_107.png|900]]

*PPT：HEX 与文本显示。*

源码包还给了控制 LED / 蜂鸣器的串口帧示例：首字节 `0x5A`、末字节 `0xA5`，中间字节控制设备状态。

**源码：`v5.1.2/15_UartPro/main/main.c`**

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "led.h"
#include "beep.h"
#include "usart.h"

void app_main(void)
{
    uint8_t length = 0 ;
    uint8_t receive_data[1024] = {0};
    led_init();
    beep_init();
    uart_init();
    while (1)
    {
        length = uart_read_bytes(UART_NUM_1, receive_data, 1024, 100);
        if(length > 0)
        {
            if(receive_data[0] == 0x5A && receive_data[length - 1] == 0xA5)
            {
                if(receive_data[1] == 0x00)
                {
                    gpio_set_level(GPIO_NUM_38,1);
                }
                else if(receive_data[1] == 0x01)
                {
                    gpio_set_level(GPIO_NUM_38,0);
                }
                if(receive_data[2] == 0x00)
                {
                    gpio_set_level(GPIO_NUM_47,1);
                }
                else if(receive_data[2] == 0x01)
                {
                    gpio_set_level(GPIO_NUM_47,0);
                }
            }
            uart_flush(UART_NUM_1);
        }
    }
}
```

这里的协议判断、索引和 GPIO 控制全部保持上传源码原样。

**源码工程：`15_Uart`、`15_UartPro`；PPT 页：97~107。**
