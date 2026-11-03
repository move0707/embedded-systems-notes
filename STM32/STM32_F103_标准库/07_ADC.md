
> **核心流程**：外部电压 → 选通道 → 采样保持 → 逐次逼近转换 → `ADC_DR` → CPU / DMA。以下以 **STM32F103C8T6、12 位 ADC、标准库（SPL）** 为准。

![[assets/ppt/s086_ADC简介.jpg]]

## 1. ADC 原理与参考电压

**ADC（模数转换器）**把引脚上的连续模拟电压变成数字。STM32F103 的 ADC 为 **12 位逐次逼近型（SAR）**，结果范围 `0～4095`，共 `2¹² = 4096` 个编码。

**参考电压**相当于 ADC 的刻度尺：输入电压占参考范围多大比例，转换结果就大致占 4096 个等级的相应比例。STM32F103C8T6 的正参考端在芯片内连接 **VDDA**，负参考端连接 **VSSA**；因此不能默认实际参考电压恰好是 3.300 V。

- 假设参考范围为 `0～3.3 V`，理想 **1 LSB（最小量化台阶）**约为 `3.3 / 4096 = 0.806 mV`。
- **分辨率**说明最小能区分多大的变化；**准确度**还取决于参考电压、噪声、模拟前端、ADC 自身误差等，不能把 0.806 mV 当作真实测量误差。

| 逐次逼近型 ADC | ADC 框图 |
| --- | --- |
| ![[assets/ppt/s087_逐次逼近型ADC.jpg]] | ![[assets/ppt/s088_ADC框图.jpg]] |

### 逐次逼近到底在做什么？

ADC **事先不知道**待测电压。内部先尝试一个数字，由内部 **DAC（数模转换电路）**产生相应的试探电压，再由**比较器（只能判断两边电压谁高谁低的硬件）**比较；**SAR（逐次逼近逻辑）**据此从高位到低位依次确定二进制结果。等效于不断缩小电压范围，**不是 CPU 执行 C 语言二分查找**。

DAC（数模转换器）：根据输入的数字编码，产生对应的模拟电压。逐次逼近不是 CPU 在计算中间值，而是 SAR 硬件逐位试探，内部 DAC 根据试探数字产生电压。

以 **4 位、参考 3.3 V、外部输入实际为 2.2 V**为例（2.2 V 是我们为了演示才知道的，ADC 并不知道）：

| 依次试探 | 试探电压 | 比较结果 | 对这一位的处理 |
| --- | ---: | --- | --- |
| `1000` | 1.6500 V | 输入更高 | 保留 1 |
| `1100` | 2.4750 V | 输入更低 | 清除刚试的位 |
| `1010` | 2.0625 V | 输入更高 | 保留 1 |
| `1011` | 2.26875 V | 输入更低 | 清除刚试的位 |

最终得到 `1010₂ = 10`。真实 F103 为 12 位，内部硬件依次确定 12 位。

![[assets/ppt/s089_ADC基本结构.jpg]]

## 2. 采样时间：为什么不能越短越好？

**采样**和**转换**是两个先后发生的阶段：

1. **采样**：ADC 接通内部开关，让外部电路给**采样保持电容**充电或放电，使电容电压尽量接近当前引脚电压。
2. **转换**：断开采样开关，SAR 根据电容保持的电压确定数字结果。

```text
传感器输出 → 外部等效电阻 → ADC 内部采样开关 → 采样电容
                                                │
                                         保持电压供 SAR 转换
```

例如外部一直是 **2 V**，但采样电容原来是 **0 V**：开关刚接通时，电容电压不可能立刻变为 2 V。如果过早断开，可能只保持了 1.7 V，后续 SAR 即使工作正常，结果也会偏低。**电容不一定每次都从 0 V 开始**；切换通道时，它也可能从上次的较高电压向当前较低电压放电。

**源阻抗**：外部信号给采样电容提供或吸收电荷时，表现出来的等效阻碍。源阻抗越大，电容通常越慢稳定；简化 RC 模型用时间常数 `τ = RC` 描述这个过程。

- **低源阻抗**：一般可以使用较短采样时间，但仍要符合芯片要求。
- **高源阻抗**（如高阻分压、电位器）：通常要延长采样时间，必要时用缓冲运放。
- **通道电压差较大时**：要注意前一通道残余电荷造成的误差。

> **选择原则**：选一个足以让采样电容稳定到所需误差范围、同时又满足采样速率的时间；不是越短越好，也不是越长一定越准确。

## 3. ADC 时钟、采样时间与采样间隔

ADC 时钟由 APB2 时钟 `PCLK2` 分频得到，**F103 ADC 时钟不得超过 14 MHz**。你的源码假设 `PCLK2 = 72 MHz`，使用 6 分频：

```c
RCC_ADCCLKConfig(RCC_PCLK2_Div6);       // fADC = 72 / 6 = 12 MHz
ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1,
                         ADC_SampleTime_55Cycles5);
```

![[assets/ppt/s097_转换时间.jpg]]

F103 的 12 位转换时间：

```text
采样时间 = 采样周期数 / fADC
一次总转换时间 = (采样周期数 + 12.5) / fADC
```
这个12.5就是逐次逼近时间。
在 `fADC = 12 MHz`、`SampleTime = 55.5 cycles` 时：

- 采样阶段：`55.5 / 12 MHz = 4.625 μs`
- SAR 转换阶段：`12.5 / 12 MHz ≈ 1.042 μs`
- **一次总转换：`5.667 μs`**

可配置的采样周期数：`1.5 / 7.5 / 13.5 / 28.5 / 41.5 / 55.5 / 71.5 / 239.5`。**采样时间是电容建立时间，不是两次测量之间的间隔**；比如程序每 10 ms 测一次，但内部单次转换仍可能只花几微秒。

## 4. Channel、Rank 与规则组

![[assets/ppt/s090_输入通道.jpg]]

- **Channel（通道）**：选择哪一个信号输入。F103C8T6 常用：`CH0～CH7 → PA0～PA7`，`CH8～CH9 → PB0～PB1`。
- **Rank（顺序位置）**：这个通道在本轮转换中排第几个。比如 `Rank1=CH2、Rank2=CH0`，就是先测 PA2，再测 PA0。
- GPIO 作为 ADC 输入要配置 `GPIO_Mode_AIN`。模拟输入时 `GPIO_Speed_50MHz` **不控制 ADC 速度**。

**规则组（Regular）**：日常转换序列，最多 16 个 Rank；**注入组（Injected）**：另一套独立触发的转换序列，最多 4 个位置，有独立的结果寄存器 `JDRx`，可用于需要特殊触发时序的采集。它们不是两套互不相关的独立 ADC 硬件。

### 四种常用转换方式

![[assets/ppt/s091_转换模式.jpg]]

> [!info]- 课程补充截图（按需展开）
> ![[Pasted image 20260916160317.png]]
> ![[Pasted image 20260916160242.png]]
> ![[Pasted image 20260916160337.png]]

- `ScanConvMode`：**一轮中是否自动按 Rank 扫多个通道**。
- `ContinuousConvMode`：**一轮结束后是否自动再来一轮**。

| 扫描 | 连续 | 触发一次后的行为 |
| --- | --- | --- |
| 关 | 关 | `CH0 → 停` |
| 关 | 开 | `CH0 → CH0 → …` |
| 开 | 关 | `CH0 → CH1 → CH2 → 停` |
| 开 | 开 | `CH0 → CH1 → CH2 → CH0 → …` |

表中通道顺序是假设的 Rank 配置，扫描**依次进行**，不等于同时采样。

## 5. 触发、EOC、DR、DMA

![[assets/ppt/s095_触发控制.jpg]]

- **触发**：告诉 ADC 现在开始一次转换或一轮扫描。可以用软件启动，也可以由支持的定时器事件按固定时刻触发。
- **`ADC_DR`**：保存最新的**规则通道**转换结果。
- **`EOC`**：End Of Conversion，转换完成标志。读取 `ADC_DR` 可以清除 EOC。
- **DMA**：硬件数据搬运机制，每个规则通道转换后，把最新 `ADC_DR` 及时搬到数组，减少 CPU 逐个读寄存器的工作。

**F103 扫描模式需要注意：** 每个通道完成时都会更新 `ADC_DR`，DMA 可逐个搬运；但 RM0008 对扫描模式的 **EOC 中断**有“最后一个通道转换结束时产生”的说明。**不要把 EOC 标志、EOC 中断和 DMA 请求当成同一件事。** 扫描结束只读一次 DR，得不到整轮所有通道的数据。

```text
Rank1=CH0 → ADC_DR → DMA → adc_buf[0]
Rank2=CH1 → ADC_DR → DMA → adc_buf[1]
Rank3=CH2 → ADC_DR → DMA → adc_buf[2]
```

F103 的 **ADC1 对应 DMA1 Channel1**。12 位右对齐数据通常使用 `uint16_t adc_buf[]`，DMA 外设/内存宽度都设为 **Half Word（16 位）**，内存地址递增。循环 DMA 中，CPU 不能随意假定正在更新的数组就是一组一致的完整测量值；应按完整序列的边界处理数据。

## 6. 对齐、校准与内部通道

![[assets/ppt/s096_数据对齐.jpg]]

**对齐**：12 位结果放到 16 位寄存器。右对齐时直接读 `0～4095`；左对齐时结果左移 4 位，不能不处理就当作 12 位原始值。

![[assets/ppt/s098_校准.jpg]]

**校准**：降低 ADC 内部电路的一部分误差，并不能校准外部传感器或修正不准确的参考电压。使能 ADC、满足上电稳定时间后执行复位校准和校准，完成再测量：

```c
ADC_Cmd(ADC1, ENABLE);
ADC_ResetCalibration(ADC1);
while (ADC_GetResetCalibrationStatus(ADC1) == SET);
ADC_StartCalibration(ADC1);
while (ADC_GetCalibrationStatus(ADC1) == SET);
```

**ADC 内部通道**
STM32F103 的 ADC1 除了测量 GPIO 引脚的电压，还可以测量两个芯片内部信号：
- `CH16`：内部温度传感器，测量芯片自身的温度变化。
- `CH17`：内部参考电压 `VREFINT`，可辅助检测 ADC 参考供电电压的变化。

使用前需要使能 `TSVREFE`，并等待内部电路稳定。
采样时间要求： 内部温度传感器约 17.1μs，VREFINT 约 5.1μs。假设 ADC 时钟为 12MHz，温度传感器可配置 239.5 cycles（约 20μs）。
内部通道和外部通道一样，最终都是通过 ADC 把模拟电压转换成数字量。

## 7. 单通道例程：PA0 → ADC1_CH0

![[assets/ppt/s099_硬件电路.jpg]]

**初始化**：开启 ADC/GPIO 时钟 → 配置 ADC 分频 → PA0 模拟输入 → 配置规则组 Rank1/采样时间 → 单次、非扫描、右对齐 → 使能/校准。

**读取一次**（教学用，正式工程应加超时/错误返回）：

```c
uint16_t AD_GetValue(void)
{
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);                 // 触发
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET); // 等待
    return ADC_GetConversionValue(ADC1);                   // 读取 DR
}
```

`ADC_RegularChannelConfig()` 只配置通道、Rank 和采样时间，**不会开始转换**。`Delay_ms(100)` 是应用程序的读取间隔，**不是 ADC 采样时间**。

原始课程代码（完整保留在源码目录）：
- [[源码/7-1 AD单通道/Hardware/AD.c|单通道 AD.c]]
- [[源码/7-1 AD单通道/User/main.c|单通道 main.c]]

## 8. 多通道例程：手动切通道 ≠ 扫描

你原课程的多通道示例仍是 **单次 + 非扫描**：每次只把新的通道放到 `Rank1`，由 CPU 逐个触发、等待、读取。

```c
uint16_t AD_GetValue(uint8_t ADC_Channel)
{
    ADC_RegularChannelConfig(ADC1, ADC_Channel, 1, ADC_SampleTime_55Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    return ADC_GetConversionValue(ADC1);
}
```

`main()` 的循环中分别读取：

```c
AD0 = AD_GetValue(ADC_Channel_0);
AD1 = AD_GetValue(ADC_Channel_1);
AD2 = AD_GetValue(ADC_Channel_2);
AD3 = AD_GetValue(ADC_Channel_3);
```

真正的**自动扫描**要预先设置多个 Rank，再开启 `ScanConvMode`；要连续采多个通道，常配 DMA。若需要**每 10 ms 获取一次多通道数据**，可用定时器触发**单次扫描 + DMA**，无需把 ADC 设成无休止连续转换。

原始课程代码：
- [[源码/7-2 AD多通道/Hardware/AD.c|多通道 AD.c]]
- [[源码/7-2 AD多通道/User/main.c|多通道 main.c]]

## 9. 电压换算与工程注意事项

假设参考电压约 3.3 V、结果右对齐：

```c
float voltage = (float)raw / 4095.0f * 3.3f; // 课程端点显示近似
// 理想量化台阶也常按 VREF / 4096 估算
```

**`4095` 是最大编码，`4096` 是编码总数**。显示电压用 4095 的端点映射是常见近似；计算理想 1 LSB 用 4096。真正要准，优先确认 **VDDA/VREF 的实际值、前端分压或放大倍数、零点偏置**。

| 现象 | 优先检查 |
| --- | --- |
| 一直读 0 / 4095 | 引脚/通道配错、输入悬空、接线或输入超量程 |
| 数值抖动 | 模拟电源、接地、PWM 干扰、前端滤波；平均只能减小部分随机噪声 |
| 高阻源或切换通道后读数偏 | 采样时间不足、源阻抗大；可延长采样或改善信号驱动 |
| 等 EOC 一直卡住 | 时钟、ADC 使能、触发配置；工程代码加超时 |
| 扫描结果顺序乱 | Rank、DMA 宽度/长度、缓冲区更新时机 |

**输入安全**：普通 ADC 模拟输入不是可直接测 5 V 的引脚。测量高于参考范围的电压需要合适的分压/保护，不能依赖 ADC 自动限幅。

**与电机项目对应**：ADC 采的是电流检测放大器的**输出电压**，不是直接采电流；通常要根据 `电流 = (测得电压 − 零点偏置) / (放大倍数 × 采样电阻)` 换算，并考虑 PWM 相位对采样结果的影响。`10 ms` 控制任务周期不等同于快速过流保护响应时间。F103 还有 **模拟看门狗（Analog Watchdog）**，可按 ADC 阈值检测越界，但不能无条件代替硬件快速保护。

### 官方资料

- [RM0008 — STM32F10xxx Reference Manual（ADC 章节）](https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-advanced-arm-based-mcus-stmicroelectronics.pdf)
- [DS5319 — STM32F103x8/xB Datasheet（ADC 电气参数，表 47；内部参考源/温度传感器参数）](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf)
- [AN2834 — How to get the best ADC accuracy in STM32 microcontrollers](https://www.st.com/resource/en/application_note/an2834-how-to-get-the-best-adc-accuracy-in-stm32-microcontrollers-stmicroelectronics.pdf)
