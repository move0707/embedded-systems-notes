# SPI

![SPI 总线](https://doc.embedfire.com/module/module_tutorial/zh/latest/_images/spi%E8%BF%9E%E6%8E%A5%E6%96%B9%E5%BC%8F.PNG)

## 时序

![SPI 时序](https://doc.embedfire.com/module/module_tutorial/zh/latest/_images/spi%E9%80%9A%E4%BF%A1%E6%97%B6%E5%BA%8F.PNG)

SPI 是真正的全双工交换：

**每来一个 SCK，MOSI 移出一位，同时 MISO 移入一位。**

所以“只读”时主机也要发送 Dummy 数据来制造时钟。

## CPOL / CPHA

CPHA=0：

![CPHA0](https://doc.embedfire.com/module/module_tutorial/zh/latest/_images/%E5%A5%87%E6%95%B0%E8%BE%B9%E6%B2%BF.PNG)

CPHA=1：

![CPHA1](https://doc.embedfire.com/module/module_tutorial/zh/latest/_images/%E5%81%B6%E6%95%B0%E8%BE%B9%E6%B2%BF.PNG)

| Mode | CPOL | CPHA | SCK空闲 | 采样 |
|---|---:|---:|---|---|
| 0 | 0 | 0 | 低 | 奇数边沿 |
| 1 | 0 | 1 | 低 | 偶数边沿 |
| 2 | 1 | 0 | 高 | 奇数边沿 |
| 3 | 1 | 1 | 高 | 偶数边沿 |

## STM32 SPI

![SPI 架构](https://doc.embedfire.com/module/module_tutorial/zh/latest/_images/spi%E6%9E%B6%E6%9E%84%E5%9B%BE.PNG)

*图源：野火 SPI 教程。重点看发送缓冲区、接收缓冲区、移位寄存器、波特率发生器和 CR/SR。*

## CubeMX

路径：

**Connectivity → SPI1**

常用：

`Mode → Full-Duplex Master`

Parameter Settings：

- Prescaler；
- Clock Polarity；
- Clock Phase；
- Data Size；
- First Bit；
- NSS。

如果外部芯片 CS 自己用 GPIO 控制，常选：

`NSS → Software`

## 初始化

```c
SPI_HandleTypeDef hspi1;

hspi1.Instance = SPI1;
hspi1.Init.Mode = SPI_MODE_MASTER;
hspi1.Init.Direction =
    SPI_DIRECTION_2LINES;
hspi1.Init.DataSize =
    SPI_DATASIZE_8BIT;
hspi1.Init.CLKPolarity =
    SPI_POLARITY_LOW;
hspi1.Init.CLKPhase =
    SPI_PHASE_1EDGE;
hspi1.Init.NSS =
    SPI_NSS_SOFT;
hspi1.Init.FirstBit =
    SPI_FIRSTBIT_MSB;

HAL_SPI_Init(&hspi1);
```

## 收发

发送：

```c
HAL_SPI_Transmit(
    &hspi1,
    tx,
    len,
    1000
);
```

接收：

```c
HAL_SPI_Receive(
    &hspi1,
    rx,
    len,
    1000
);
```

最符合 SPI 本质的是：

```c
HAL_SPI_TransmitReceive(
    &hspi1,
    tx,
    rx,
    len,
    1000
);
```

## CS

软件 NSS 时：

```c
HAL_GPIO_WritePin(
    CS_GPIO_Port,
    CS_Pin,
    GPIO_PIN_RESET
);

HAL_SPI_TransmitReceive(...);

HAL_GPIO_WritePin(
    CS_GPIO_Port,
    CS_Pin,
    GPIO_PIN_SET
);
```

`HAL_SPI_Transmit()` 不会自动理解你外接 W25Q64 的 CS 是哪一个普通 GPIO。

## IT / DMA

```c
HAL_SPI_Transmit_IT(...)
HAL_SPI_Receive_IT(...)
HAL_SPI_TransmitReceive_IT(...)

HAL_SPI_Transmit_DMA(...)
HAL_SPI_Receive_DMA(...)
HAL_SPI_TransmitReceive_DMA(...)
```

## 对照

| 标准库 | HAL |
|---|---|
| `SPI_Init()` | `HAL_SPI_Init()` |
| `SPI_I2S_SendData()` | `HAL_SPI_Transmit()` |
| `SPI_I2S_ReceiveData()` | `HAL_SPI_Receive()` |
| 手写 SwapByte | `HAL_SPI_TransmitReceive()` |
