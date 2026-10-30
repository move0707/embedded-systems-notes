# STM32 HAL 常用函数速查表

> STM32F103 + HAL库  
> 用于日常开发快速查询函数名

---

# 1. GPIO

## GPIO初始化

```c
MX_GPIO_Init();
```

作用：

初始化 GPIO 外设（CubeMX自动生成）

---

## GPIO输出电平

```c
HAL_GPIO_WritePin(
    GPIO_TypeDef *GPIOx,
    uint16_t GPIO_Pin,
    GPIO_PinState PinState
);
```

示例：

```c
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
```

作用：

设置GPIO输出高低电平。

---

## GPIO翻转

```c
HAL_GPIO_TogglePin(
    GPIO_TypeDef *GPIOx,
    uint16_t GPIO_Pin
);
```

示例：

```c
HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);
```

作用：

高电平变低电平，低电平变高电平。

---

## GPIO读取

```c
HAL_GPIO_ReadPin(
    GPIO_TypeDef *GPIOx,
    uint16_t GPIO_Pin
);
```

示例：

```c
if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0)==GPIO_PIN_SET)
{

}
```

返回：

```c
GPIO_PIN_SET
GPIO_PIN_RESET
```

---

# 2. 延时

## 毫秒延时

```c
HAL_Delay(uint32_t Delay);
```

示例：

```c
HAL_Delay(1000);
```

作用：

阻塞延时1000ms。

---

# 3. UART串口

## 初始化

```c
MX_USART1_UART_Init();
```

作用：

初始化USART。

---

## 串口发送

```c
HAL_UART_Transmit(
    UART_HandleTypeDef *huart,
    uint8_t *pData,
    uint16_t Size,
    uint32_t Timeout
);
```

示例：

```c
uint8_t data[]="hello";

HAL_UART_Transmit(
    &huart1,
    data,
    sizeof(data),
    HAL_MAX_DELAY
);
```

作用：

发送串口数据。

---

## printf重定向

```c
int _write(
    int file,
    char *ptr,
    int len
)
{
    HAL_UART_Transmit(
        &huart1,
        (uint8_t*)ptr,
        len,
        HAL_MAX_DELAY
    );

    return len;
}
```

作用：

让printf通过UART输出。

---

# 4. TIM基础定时器

## 初始化

```c
MX_TIMx_Init();
```

作用：

初始化定时器。

---

## 启动定时器

```c
HAL_TIM_Base_Start(
    TIM_HandleTypeDef *htim
);
```

---

## 启动定时器中断

```c
HAL_TIM_Base_Start_IT(
    TIM_HandleTypeDef *htim
);
```

---

## 停止定时器中断

```c
HAL_TIM_Base_Stop_IT(
    TIM_HandleTypeDef *htim
);
```

---

## 设置ARR

```c
__HAL_TIM_SET_AUTORELOAD(
    &htim,
    value
);
```

作用：

修改自动重装载值。

---

## 设置CNT

```c
__HAL_TIM_SET_COUNTER(
    &htim,
    value
);
```

作用：

修改当前计数值。

---

## 获取CNT

```c
__HAL_TIM_GET_COUNTER(
    &htim
);
```

作用：

读取当前计数器值。

---

## 获取系统时间

```c
HAL_GetTick();
```

返回：

系统运行时间(ms)。

常用于：

- 超时判断
- 按键消抖
- 定时任务

---

# 5. PWM输出

## 开启PWM

```c
HAL_TIM_PWM_Start(
    &htim,
    TIM_CHANNEL_x
);
```

示例：

```c
HAL_TIM_PWM_Start(
    &htim3,
    TIM_CHANNEL_1
);
```

---

## 修改PWM占空比

```c
__HAL_TIM_SET_COMPARE(
    &htim,
    Channel,
    Compare
);
```

示例：

```c
__HAL_TIM_SET_COMPARE(
    &htim3,
    TIM_CHANNEL_1,
    500
);
```

作用：

修改CCR值。

公式：

```
占空比 ≈ CCR / (ARR+1)
```

---

# 6. 输入捕获

## 开启输入捕获

```c
HAL_TIM_IC_Start(
    &htim,
    TIM_CHANNEL_x
);
```

作用：

开启输入捕获功能。

---

## 停止输入捕获

```c
HAL_TIM_IC_Stop(
    &htim,
    TIM_CHANNEL_x
);
```

---

## 读取捕获值(CCR)

```c
HAL_TIM_ReadCapturedValue(
    &htim,
    TIM_CHANNEL_x
);
```

作用：

读取捕获寄存器CCR。

---

## 判断捕获事件

```c
__HAL_TIM_GET_FLAG(
    &htim,
    FLAG
);
```

示例：

```c
__HAL_TIM_GET_FLAG(
    &htim2,
    TIM_FLAG_CC1
);
```

作用：

判断是否捕获完成。

---

## 清除捕获标志

```c
__HAL_TIM_CLEAR_FLAG(
    &htim,
    FLAG
);
```

---

# 7. 编码器模式

## 开启编码器

```c
HAL_TIM_Encoder_Start(
    &htim,
    TIM_CHANNEL_ALL
);
```

作用：

启动定时器编码器接口。

---

## 读取编码器位置

```c
__HAL_TIM_GET_COUNTER(
    &htim
);
```

作用：

读取编码器CNT。

---

## 设置编码器位置

```c
__HAL_TIM_SET_COUNTER(
    &htim,
    value
);
```

作用：

编码器归零。

---

## 判断计数方向

```c
__HAL_TIM_IS_TIM_COUNTING_DOWN(
    &htim
);
```

返回：

```
1：向下计数
0：向上计数
```

---

# 8. ADC

## 初始化

```c
MX_ADC1_Init();
```

---

## ADC校准

```c
HAL_ADCEx_Calibration_Start(
    &hadc
);
```

---

## 开始ADC转换

```c
HAL_ADC_Start(
    &hadc
);
```

---

## 等待转换完成

```c
HAL_ADC_PollForConversion(
    &hadc,
    Timeout
);
```

---

## 获取ADC值

```c
HAL_ADC_GetValue(
    &hadc
);
```

返回：

ADC转换结果。

---

# 9. 中断回调函数

## 定时器中断

```c
HAL_TIM_PeriodElapsedCallback(
    TIM_HandleTypeDef *htim
);
```

作用：

定时器周期溢出执行。

---

## GPIO外部中断

```c
HAL_GPIO_EXTI_Callback(
    uint16_t GPIO_Pin
);
```

作用：

外部中断触发执行。

---

# 10. DMA

## UART DMA发送

```c
HAL_UART_Transmit_DMA();
```

作用：

DMA方式发送串口数据。

---

# 11. 常用CubeMX生成函数

|函数|作用|
|-|-|
|MX_GPIO_Init()|GPIO初始化|
|MX_ADC1_Init()|ADC初始化|
|MX_TIMx_Init()|定时器初始化|
|MX_USARTx_UART_Init()|UART初始化|
|MX_SPIx_Init()|SPI初始化|
|MX_I2Cx_Init()|I2C初始化|

---

# 12. 定时器三个核心理解

## PWM

```
CNT和CCR比较
↓
输出PWM
```

---

## 输入捕获

```
外部边沿
↓
保存CNT到CCR
↓
计算时间
```

---

## 编码器

```
A/B信号
↓
硬件判断方向
↓
修改CNT
↓
读取位置
```

---

# 13. 常用数据类型

|类型|大小|范围|
|-|-|-|
|uint8_t|8位|0~255|
|uint16_t|16位|0~65535|
|uint32_t|32位|0~4294967295|
|int16_t|16位|-32768~32767|
|float|浮点数|小数|

---

# 最常查函数

```text
GPIO:
HAL_GPIO_WritePin()
HAL_GPIO_ReadPin()

UART:
HAL_UART_Transmit()
_write()

TIM:
HAL_TIM_Base_Start_IT()
HAL_GetTick()

PWM:
HAL_TIM_PWM_Start()
__HAL_TIM_SET_COMPARE()

输入捕获:
HAL_TIM_IC_Start()
HAL_TIM_ReadCapturedValue()

编码器:
HAL_TIM_Encoder_Start()
__HAL_TIM_GET_COUNTER()

ADC:
HAL_ADC_Start()
HAL_ADC_GetValue()
```







# 第一类：输入模式（让单片机“感知”外部世界）

当引脚配置为输入时，单片机不向外输出电压，而是去“读取”外面的电压是高（1）还是低（0）。

**1. 浮空输入 (Floating Input) —— 也就是你说的“悬空”**

- **通俗解释：** 芯片内部什么都不接，就像一片随风飘摇的树叶。外面给什么电压，它就读到什么电压。
- **缺点：** 极度敏感！如果外面什么都没接，空气中的静电或者旁边的电磁干扰，都会让它读到的数据在 0 和 1 之间疯狂横跳。
- **应用场景：** 通常用于外部已经有了明确的上/下拉电阻的电路中，或者作为某些特殊通信协议的引脚。

**2. 下拉输入 (Pull-down Input)**

- **通俗解释：** 芯片内部接了一个电阻连到“地”（GND）。就像给树叶绑了一个沙袋。
- **效果：** 如果外面不接线（悬空），它就会被这个“沙袋”稳稳地拉在**低电平（0）**。只有外面输入一个强势的**高电平（1）**，才能把数值拉上去。
- **应用场景：** 就像你刚才看到的 TB6612 的 `AIN1` 和 `AIN2`，默认下拉能防止未接线时电机乱转。

**3. 上拉输入 (Pull-up Input)**

- **通俗解释：** 和下拉相反，芯片内部接了一个电阻连到电源（VCC）。就像给树叶绑了一个氢气球。
- **效果：** 如果外面不接线，它会默认飘在**高电平（1）**。只有外部输入一个强力的**低电平（0）**（比如按下一个接地的按键），它才会变成 0。
- **应用场景：** 最常用于连接按键。按键没按下时是 1，按下时导通接地变成 0。

**4. 模拟输入 (Analog Input)**

- **通俗解释：** 前面三种只能读 0 和 1（非黑即白）。模拟输入则是绕开数字逻辑，直接读取真实的电压“模拟量”（比如 1.5V、3.3V）。
- **应用场景：** 读取温度传感器、光敏电阻、电位器（旋钮）的连续变化数值。

### 第二类：输出模式（让单片机去“控制”外部世界）

当引脚配置为输出时，单片机负责主动输出电压去驱动别人。

**5. 推挽输出 (Push-Pull Output) —— 最最常用的输出**

- **通俗解释：** 内部有两个开关（晶体管），一个负责推（接高电平），一个负责挽（拉低电平）。它就像一个强壮的工人，既能用力往上举（输出强力的 1），也能用力往下拉（输出强力的 0）。
- **应用场景：** **绝大多数的控制都用它！** 比如点亮 LED 灯、控制继电器，或者发送指令给 TB6612 的 `AIN1` 和 `AIN2` 来控制电机转向。

**6. 开漏输出 (Open-Drain Output)**

- **通俗解释：** 这个工人只有“向下拉”的力气，没有“向上推”的力气。它可以输出强力的低电平（0），但如果让它输出高电平（1），它就会直接“摆烂”变成浮空状态。
- **怎么补救：** 如果你想用它输出 1，必须在外部电路接一个上拉电阻，借用外部电源的力量把电平拉高。
- **应用场景：** 用于 I2C 通信总线，或者用来控制比单片机工作电压更高的设备（比如 3.3V 的单片机通过外接上拉电阻去控制 5V 的电路）。

### 第三类：复用模式（单片机内部的高级功能托管）

前 6 种模式，都是我们在代码里手动写 `输出 1` 或 `读取 0`（这叫通用 GPIO）。但有时候，引脚需要交给单片机内部的“硬件专员”来接管（比如专门生成 PWM 波的定时器，或者串口通信模块）。

**7. 复用推挽输出 (Alternate Function Push-Pull)**

- **通俗解释：** 引脚的电气特性和“推挽输出”一模一样。区别在于，这个引脚现在不由你写的普通代码控制了，而是外包给了内部的硬件外设（比如专门输出 PWM 调速信号给 TB6612 的 `PWMA`）。

**8. 复用开漏输出 (Alternate Function Open-Drain)**

- **通俗解释：** 引脚电气特性同“开漏输出”，控制权同样交给了内部硬件专员。最典型的就是 I2C 通信的 SCL 和 SDA 引脚。









# STM32 定时器与 PWM 波形生成原理

使用的是 **STM32 单片机**（以 STM32F103 使用内部时钟 72MHz 为例）。STM32 内部有专门的“定时器”硬件来生成 PWM 波，主要需要配置以下四个核心寄存器参数。

> **💡 核心类比**
> 把定时器想象成一个“水管、减速齿轮和水桶”的系统：

#### 1. 寄存器参数解析

*   **CK_PSC (预分频器的输入时钟) —— 总水源**
    *   对于 STM32F103，这个主频通常是固定的 **72MHz**（即 72,000,000 Hz）。
*   **PSC (预分频系数) —— 减速齿轮**
    *   72MHz 太快了，需要用 PSC 把它降速。
*   **ARR (自动重装载值 / 计数个数) —— 水桶的容量**
    *   定时器会从 0 开始数数，数到 ARR 的值就会清零重新开始。数完一桶的时间，就是 **PWM 的一个周期（频率）**。
*   **CCR (比较值) —— 水桶里的警戒线**
    *   *（注：图中写成了 CRR 为笔误，官方手册标准简称为 CCR）*
    *   在往水桶注水的过程中，水位低于 CCR 时输出高电平，高于 CCR 时输出低电平。这条线的高低，决定了**占空比（电机转速）**。

---

### 2. 公式与实战计算演练

以下展示如何反推这些寄存器的值。

**🎯 目标配置：** 生成一个频率 **10KHz**，占空比 **50%**，分辨率 **1%** 的 PWM 波。

#### 第一步：确定 ARR（决定分辨率）
*   **参数含义：** 分辨率 1% 意思是想把电机的速度分为 100 个挡位（1% 到 100%）。
*   **公式：** Reso = 1 / (ARR + 1)
*   **计算：** 要求 1 / (ARR + 1) = 1%，那么解得 ARR + 1 = 100。这意味着水桶容量设定为 100。

#### 第二步：确定 CCR（决定占空比）
*   **参数含义：** 占空比 50% 意思是想让电机以一半的最大速度转动。
*   **公式：** Duty = CCR / (ARR + 1)
*   **计算：** 已知分母是 100，要求结果是 50%，所以警戒线 CCR = 50。

#### 第三步：确定 PSC（决定最终频率）
*   **参数含义：** 目标频率为 10KHz。
*   **公式：** Freq = CK_PSC / ((PSC + 1) * (ARR + 1))
*   **代入已知条件：** 10000 = 72000000 / ((PSC + 1) * 100)
*   **化简：** 10000 = 720000 / (PSC + 1)
*   **解出：** PSC + 1 = 72





# 重点维护makefile

```
TARGET

C_SOURCES

ASM_SOURCES

C_DEFS

C_INCLUDES

LDSCRIPT
```

