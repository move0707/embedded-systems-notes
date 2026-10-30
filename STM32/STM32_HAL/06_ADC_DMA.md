# ADC + DMA

![ADC 功能框图](https://doc.embedfire.com/mcu/stm32/f103zhinanzhe/hal/zh/latest/_images/image212.png)

*图源：野火 ADC 章节。顺着 1→8 看就是：输入范围 → 通道 → 序列 → 触发 → ADC_CLK → 数据寄存器 → 中断/DMA。*

![ADC 通道](https://doc.embedfire.com/mcu/stm32/f103zhinanzhe/hal/zh/latest/_images/image39.png)

## ADC 为什么常配 DMA

野火 ADC 章节明确指出：规则组可以有多个通道，但规则数据寄存器 `ADC_DR` 只有一个。

所以多通道扫描时：

**CH0 → DR → CH1 → DR → CH2 → DR**

如果不及时搬走，前面的值会被覆盖。

![DMA 框图](https://doc.embedfire.com/mcu/stm32/f103/hal_generalzh/latest/_images/image28.png)

![DMA1 请求映射](https://doc.embedfire.com/mcu/stm32/f103/hal_generalzh/latest/_images/image37.png)

F103 的 DMA 通道不是随便选的，要看外设请求映射。

## CubeMX：ADC

路径：

**Analog → ADC1**

先在 Pinout 启用：

`IN0 / IN1 / ...`

然后：

**Parameter Settings**

重点看：

- Scan Conversion Mode；
- Continuous Conversion Mode；
- Number Of Conversion；
- Rank；
- Sampling Time；
- Data Alignment。

## CubeMX：DMA

ADC 页面：

**DMA Settings → Add**

F103 ADC1 常对应固定 DMA 通道映射，CubeMX 会按芯片资源选择。

重点检查：

- Direction：Peripheral to Memory；
- Peripheral Increment：Disable；
- Memory Increment：Enable；
- Mode：Normal / Circular；
- 数据宽度。

## 单次 Polling

```c
HAL_ADC_Start(&hadc1);

HAL_ADC_PollForConversion(
    &hadc1,
    100
);

uint32_t value =
    HAL_ADC_GetValue(&hadc1);

HAL_ADC_Stop(&hadc1);
```

对应标准库：

```c
ADC_SoftwareStartConvCmd()
→ 等 EOC
→ ADC_GetConversionValue()
```

## 通道

HAL：

```c
ADC_ChannelConfTypeDef sConfig = {0};

sConfig.Channel = ADC_CHANNEL_0;
sConfig.Rank = ADC_REGULAR_RANK_1;
sConfig.SamplingTime =
    ADC_SAMPLETIME_55CYCLES_5;

HAL_ADC_ConfigChannel(
    &hadc1,
    &sConfig
);
```

标准库：

```c
ADC_RegularChannelConfig(...)
```

## 校准

F1 HAL 常用：

```c
HAL_ADCEx_Calibration_Start(
    &hadc1
);
```

标准库原来要 Reset Calibration、等待，再 Start Calibration、再等待。

## ADC + DMA

```c
uint32_t adc_buf[4];

HAL_ADC_Start_DMA(
    &hadc1,
    adc_buf,
    4
);
```

CubeMX 已经生成 DMA Handle、通道、方向、宽度、自增、NVIC 关联时，应用层只需启动。

停止：

```c
HAL_ADC_Stop_DMA(&hadc1);
```

## Callback

```c
void HAL_ADC_ConvCpltCallback(
    ADC_HandleTypeDef *hadc)
{
}
```

DMA 循环采样时还要认识 Half Complete 思路，方便一半 Buffer 到达时提前处理。

## 对照

| 标准库 | HAL |
|---|---|
| `ADC_Init()` | `HAL_ADC_Init()` |
| `ADC_RegularChannelConfig()` | `HAL_ADC_ConfigChannel()` |
| `ADC_SoftwareStartConvCmd()` | `HAL_ADC_Start()` |
| `ADC_GetFlagStatus()` | `HAL_ADC_PollForConversion()` |
| `ADC_GetConversionValue()` | `HAL_ADC_GetValue()` |
| `DMA_Init + ADC_DMACmd` | CubeMX DMA + `HAL_ADC_Start_DMA()` |
