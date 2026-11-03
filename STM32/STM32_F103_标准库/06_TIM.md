# TIM 定时器｜STM32F103（我的复习笔记）

> **怎么读：** 先看第 1～4 章，把图和缩写弄明白，再看下面的 8 个课程实验。**TIM 的核心就是 CNT 在计数**：有人给它计数节拍，有人规定什么时候结束，有人用当前计数值做比较或捕获。不要一开始就死记函数。
>
> **芯片范围：** 主要按我使用的 **STM32F103C8T6** 和 STM32 标准库讲解。资料中有时展示整个 F103 系列的功能，并不意味着 C8T6 每个定时器都有这些功能。
>
> **源码：** 原课程 6-1～6-8 的完整 C 代码放在各实验下面，**可以直接看，不用点源文件链接**。需要特别注意的旧注释和工程问题，会在代码附近提醒；原代码是课程示例，不等于成品项目代码。

## 1. 先认识 TIM：它是什么，分哪几种？

### 1.1 TIM 到底干什么？

TIM（Timer）就是 STM32 里的**硬件计数器**。它不用 CPU 一直执行 `i++`，只要初始化好，就能自己根据某种信号改变 `CNT`。

它能做的事情，大致分成三类：

1. **按时间工作：** 例如每 10 ms 产生一次事件、输出 20 kHz PWM。
2. **记录外部信号：** 例如统计传感器脉冲、测方波的频率和占空比。
3. **配合其他硬件：** 例如读取编码器 A/B 相，或在固定时间触发 ADC。

所以不要把“定时器”理解成只能定时，它本质上更像**带有输入选择、比较器和触发功能的计数设备**。

![[assets/ppt/s052_TIM简介.jpg]]

### 1.2 基本定时器、通用定时器、高级定时器有什么不同？

这三种是按**功能多少**分的，不是按“谁计时更准确”分的。

| 类型 | 最好理解成 | 主要能做什么 | STM32F103C8T6 上有没有 |
|---|---|---|---|
| **基本定时器（Basic）** | 只负责打节拍的简化定时器 | PSC/CNT/ARR、更新事件、中断、部分型号可输出内部触发；一般**没有 CH 输入输出通道** | **没有 TIM6、TIM7** |
| **通用定时器（General-purpose）** | 打节拍 + 测外部信号 + 输出波形 | 更新中断、输入捕获、输出比较、PWM、部分外部计数/编码器、触发同步 | **TIM2、TIM3、TIM4** |
| **高级控制定时器（Advanced-control）** | 通用 TIM 的基础上，为功率控制增加专用功能 | 互补 PWM、死区、Break 故障关断、重复计数器等 | **TIM1** |

**注意：** 有些 STM32F103 型号还具有 TIM8、TIM6/7 等，不能拿别的开发板的课程直接套到自己的 C8T6 上。

| 通用定时器结构 | 基本定时器结构 |
|---|---|
| ![[assets/ppt/s055_通用定时器.jpg]] | ![[assets/ppt/s056_基本定时器.jpg]] |

#### 基本定时器为什么还要单独存在？

如果一个任务只需要“每隔一段时间产生一次更新事件”，就不需要输入捕获、PWM 输出引脚等大量硬件。基本定时器就是做这种事情的精简结构。

比如某些 STM32 型号可以用 TIM6 定时触发 DAC；但**我这块 F103C8T6 没有 TIM6**，需要改用芯片支持的其他定时器和触发关系。

#### 通用定时器为什么用途这么多？

因为它除了 `PSC/CNT/ARR`，还有多个 `CHx` 通道，以及 `CCRx` 捕获/比较寄存器。

- **输出比较**：CNT 数到某个值，控制输出动作，比如 PWM。
- **输入捕获**：外部出现一个上升沿，硬件把当时 CNT 的数字保存下来。
- **编码器**：把两路外部信号的变化当作计数依据，还能判断正反方向。

后面的课程实验主要围绕通用 TIM2/TIM3 展开。

#### 高级定时器“高级”在哪里？特别是 TIM1

TIM1 也能做普通 PWM、输入捕获等，但它多了针对电机功率级的东西：

| 名称 | 大白话 | 为什么有用 |
|---|---|---|
| **互补输出 CHx / CHxN** | 同一个 PWM 通道可以产生配合使用的两路输出 | 方便控制半桥上、下管的驱动信号；例如 CH1 和 CH1N |
| **死区 Dead Time** | 上管关掉后，故意等一小会儿再让下管开 | 防止上、下功率管切换时同时导通造成直通 |
| **Break / BKIN** | 外部故障信号进来，定时器硬件按配置迅速改变 PWM 输出 | 过流/驱动器报错时，减少等待软件中断的时间 |
| **RCR 重复计数器** | 允许经过若干次计数周期，再产生符合设置的更新事件 | 配合 PWM 周期、更新频率安排更复杂的控制时序 |
| **MOE 主输出使能** | 给高级定时器输出再加一道总开关 | TIM1 就算 CNT、CCR 正常运行，也可能因为 MOE 未打开而不输出 PWM |

**对我之前的电机项目：** 如果只是让一个电机驱动模块的 PWM 输入脚接收普通单路 PWM，TIM2/3/4 也能做；如果直接控制半桥/三相功率开关，就需要重点研究 TIM1 的互补输出、死区和 Break。注意硬件保护还要结合驱动器和电路设计，不能只靠软件配置。

### 1.3 先建立一个统一印象

```text
定时器用什么数？       → 时钟或外部有效脉冲
现在数到哪里了？       → CNT
数多快？               → PSC
一轮数到哪里结束？     → ARR
什么时候改变输出？     → CCR + 输出比较（OC）
外部边沿发生在何时？   → 输入捕获（IC）把 CNT 存到 CCR
如何通知 CPU？         → 更新/捕获等中断
如何通知其他外设？     → TRGO、定时器间触发
```

接下来就按这个顺序认识结构图里的词。

---

## 2. 先看懂定时器结构图：每个缩写代表什么？

这部分要放在最前面。否则图上出现 `ETR`、`TRGI`、`ITR`，没解释就去讲外部时钟，很容易越看越乱。

### 2.1 把整张图拆成三段

```text
【左边：信号从哪里来】
   RCC 内部时钟 / 外部引脚 ETR / 定时器输入 TI1、TI2 / 其他定时器 ITRx
                     ↓
【中间：选什么来控制计数】
   选时钟源、选触发源、决定要计数/复位/门控/启动
                     ↓
【右边：真正计数并产生结果】
       PSC → CNT（受 ARR 控制）
                     ↓
      更新事件 / 捕获比较 / 中断 / TRGO
```

![[assets/ppt/s057_时基单元.jpg]]

**图里同时画出几条路线，是在说明“这个 TIM 有多种可选功能”，不表示所有计数模式同时启动。**

### 2.2 最容易搞混的一组：时钟、计数节拍、触发

这三个词不是一个意思。

- **时钟（Clock）**：不断出现的规则节拍，例如来自 RCC 的 72 MHz。
- **计数节拍**：最终真正让 `CNT` 加 1（或减 1）的那次有效事件。
- **触发（Trigger）**：发生某个条件时让硬件执行动作，动作可以是启动、复位、计数或发送信号给 ADC，并不一定代表 CNT 要加 1。

举例：**输入捕获测频**时，CNT 可以一直用内部 1 MHz 时钟计时间，外部上升沿只负责“此刻记下 CNT”，也可以进一步把 CNT 清零。外部上升沿是触发事件，但不是 CNT 的 1 MHz 计数时钟。

### 2.3 一张表认识 TIM 常见的信号名称

| 缩写 | 中文意思 | 你应该怎么理解 |
|---|---|---|
| **RCC** | 复位和时钟控制 | 给 TIM 外设送时钟，并控制 TIM 外设时钟使能 |
| **CK_INT** | TIM 内部时钟输入 | 从 RCC 时钟树进入 TIM 的计数时钟来源 |
| **CK_PSC** | 预分频器输入时钟 | 选好计数来源后，进入 PSC 之前的信号 |
| **CK_CNT** | 计数器时钟 | PSC 分频后，真正推动 CNT 计数的节拍 |
| **PSC** | Prescaler | 预分频器：把时钟变慢（除以 PSC+1） |
| **CNT** | Counter | 计数器当前值，不断加、减或按编码器方向变 |
| **ARR** | Auto-reload Register | 自动重装载值：规定计数的周期/范围 |
| **CCR1～CCR4** | Capture/Compare Register | 输出时作为比较值；输入捕获时保存某一时刻的 CNT |
| **CH1～CH4** | Timer Channel | 定时器输入/输出通道，连接到芯片支持的对应引脚 |
| **TI1、TI2** | Timer Input | 定时器的通道输入信号，例如 TI1 来自 CH1 所接的引脚路径 |
| **ETR** | External Trigger | 外部触发输入路径，可用来驱动外部时钟模式 2 或参与触发选择 |
| **ETRF** | Filtered ETR | ETR 经过极性/预分频/滤波等处理后的内部信号 |
| **ITR0～ITR3** | Internal Trigger | 芯片内部“别的定时器传来的触发信号”，具体映射由芯片规定 |
| **TRGI** | Trigger Input | **本定时器接收到、已经选定的触发信号**，是内部信号，不是额外的 GPIO |
| **TRGO** | Trigger Output | **本定时器向外发出的触发信号**，可给另一 TIM 或支持它的 ADC 等外设 |
| **UEV** | Update Event | 更新事件，例如正常计数溢出时产生；也可能由软件触发 |
| **UIF** | Update Interrupt Flag | 更新标志：记录更新发生的状态；不等于一定进入 CPU 中断 |
| **NVIC** | 中断控制器 | 决定符合条件的外设中断请求是否能让 CPU 进入中断函数 |

这是术语总表，下面只重点讲最容易混淆的几个词。

### 2.4 ETR 到底是什么？和 GPIO、TI1 有什么关系？

`ETR` 的完整名字叫 **External Trigger（外部触发输入）**。

通俗讲：**TIM 留出一条特定的硬件入口，可以接收外部引脚送来的脉冲/触发信号。**

以课程用的 TIM2 默认映射为例，TIM2_ETR 对应 **PA0**（实际要结合具体引脚映射配置和芯片封装）。

```text
PA0 上的外部脉冲
        ↓
    TIM2_ETR
        ↓
极性选择/外部预分频/滤波
        ↓
       ETRF
        ↓
可以进入“外部时钟模式2”，推动 PSC/CNT 计数
也可以作为某些触发选择路径的来源
```

- `ETR`：外部信号进入的路径。
- `ETRF`：经过内部处理后、能进一步参与控制的信号。
- `ETP`：ETR 的极性配置，不是另外一个叫 ETP 的引脚。

**ETR 不是“所有外部信号”的总称。** `TI1/TI2` 同样可能连着外部 GPIO，但它们是另外的输入路径。PA0 在 TIM2 默认映射里也对应 TIM2_CH1，**同一个物理引脚可能具有不同的复用功能，不能因此认为 ETR 与 CH1 是两个不同的 GPIO。**

### 2.5 TIx、TI1FP1、TI2FP2 到底是什么？

`TI` 可以先理解成 **Timer Input（定时器输入）**。例如：

- `TI1`：定时器通道 1 的输入路径。
- `TI2`：定时器通道 2 的输入路径。
- `TI1FP1`：通道 1 输入经过采样/滤波/极性选择等处理后，送到后续触发或捕获控制的信号。
- `TI2FP2`：通道 2 的类似信号。

你可以把它理解成：

```text
外部引脚（例如 TIM3_CH1 默认 PA6）
                   ↓
                   TI1  输入电路
                   ↓
                输入滤波器
                   ↓
        极性/边沿处理等输入通道逻辑
                   ↓
                 TI1FP1
                   ↓
           供捕获、触发选择等使用
```

这里的 `F` 主要提醒你信号经过了滤波相关的输入路径，`P` 与极性处理有关；不是额外拉出一根叫 `TI1FP1` 的线。学习时先记住**信号经过输入处理后的名字**就够了。

还有一个 `TI1F_ED`：表示从 TI1 处理路径中检测边沿的触发信号，可用于特定触发选择。它不等于 `TI1FP1`；它用于按配置响应边沿，具体行为看手册。

### 2.6 ITRx：不是 GPIO，是定时器之间的内部连接

`ITR` = Internal Trigger，内部触发。

例如想让 TIM2 和 TIM3 协同运行，不一定需要拿一根导线把两个定时器的 GPIO 引脚接起来。芯片内部已经设计了若干触发连接。

```text
TIM2 按自己的时钟计数
          ↓
TIM2 产生 TRGO
          ↓
芯片内部的触发连接
          ↓
TIM3 某一路 ITRx
          ↓
TIM3 把这路选为 TRGI
          ↓
TIM3 根据从模式执行启动、复位、门控或计数
```

**`ITR0～ITR3` 是“本定时器可选的内部触发输入编号”，不是永远对应 TIM1/TIM2/TIM3/TIM4 的固定顺序。** 必须按 RM0008 中各 TIM 的内部触发映射表选。

### 2.7 TRGI 与 TRGO：名字相似，方向相反

这两个词要放一起记：

| 名称 | 相对当前这个 TIM 的方向 | 大白话 |
|---|---|---|
| **TRGI = Trigger Input** | **进来** | “别人给我一个触发信号，我用它做什么？” |
| **TRGO = Trigger Output** | **出去** | “我产生事件，要不要通知别的外设？” |

例如：

```text
    TIM2（发送者）                     TIM3（接收者）
更新事件 → TIM2_TRGO → 内部 ITRx → TIM3_TRGI
                                          ↓
                                TIM3 根据模式复位/启动/计数
```

**注意：TRGI 不是某一个独立引脚，也不等于 ETR。** 本定时器可以从 `ITRx`、`TI1FP1`、`TI2FP2`、`ETRF` 等支持的来源中选一路作为 TRGI。选什么信号，和让这个信号干什么，是两步配置。

### 2.8 TS、SMS、MMS：选哪路？收到后做什么？发出去什么？

这三个字段不需要死背二进制值，但一定要知道分工。

| 控制字段 | 位于哪里 | 决定什么 |
|---|---|---|
| **TS（Trigger Selection）** | `TIMx_SMCR` | **选哪一路输入信号作为 TRGI**，如 `TI1FP1`、`ETRF`、`ITRx` |
| **SMS（Slave Mode Selection）** | `TIMx_SMCR` | **选中的 TRGI 到来后怎样控制本 TIM**，如 Reset、Gated、Trigger、External Clock Mode 1 |
| **MMS（Master Mode Selection）** | `TIMx_CR2` | **本 TIM 把什么事件送到 TRGO**，如更新事件 Update |

下面这段课程代码：

```c
TIM_SelectInputTrigger(TIM3, TIM_TS_TI1FP1);
TIM_SelectSlaveMode(TIM3, TIM_SlaveMode_Reset);
```

读成两句大白话即可：

1. **TS**：让 TIM3 把 TI1FP1 选为输入触发 TRGI。
2. **SMS**：这个触发出现时，把 TIM3 的 CNT 复位，以便测下一次边沿间隔。

在这个例子里，**CNT 仍然可以使用内部 1 MHz 时钟**。输入触发负责复位，而不是负责每次加 1！这是这章最容易混的地方。

---

## 3. 一个 TIM 到底选哪种计数方式？

看图容易以为内部时钟、外部时钟 1、外部时钟 2、编码器都同时给 CNT 计数。**不是。它们是不同的计数配置方式。**

### 3.1 内部时钟模式：经过固定时间，CNT 就加 1

```text
RCC → CK_INT → CK_PSC → PSC → CK_CNT → CNT
```

比如定时中断、PWM 输出、用输入捕获测方波时间间隔，都经常使用内部时钟模式。没有外部脉冲也照样计数。

### 3.2 外部时钟模式 1：把选中的 TRGI 当作计数来源

常见路径是：

```text
外部引脚 → TI1 → 输入滤波/极性 → TI1FP1
                                      ↓
                             TS 选择为 TRGI
                                      ↓
                         SMS = 外部时钟模式1
                                      ↓
                               CK_PSC → PSC → CNT
```

这时某个有效外部脉冲/边沿可以成为计数节拍。来源不只限于 TI1；受支持的 `TI2FP2`、`ETRF` 或 `ITRx` 等也可以作为选定触发路径，具体要符合当前 TIM 的 TS 选项。

**不要把“外部时钟模式1”理解成“一定从 GPIO 来”。** 如果它选的是 `ITRx`，实际来源可能是芯片内部另一个 TIM。

### 3.3 外部时钟模式 2：专门走 ETR 路径

```text
外部引脚 → ETR → 极性/滤波/外部分频 → ETRF
                                          ↓
                                     CK_PSC → PSC → CNT
```

课程实验 6-2 就是这个模式，用 TIM2 的 ETR 默认映射引脚 PA0 统计外部脉冲。常见的 F103 控制是配置 `ECE` 使能这条外部计数路径；不等于把 TIM 配成输入捕获。

**模式1与模式2最重要的区别**：模式1主要走 **TRGI + 从模式选择**；模式2主要走 **ETR 专用外部时钟路径**。不必把它们想成“一个更高级，一个更低级”，按实际输入来源和功能选即可。

### 3.4 编码器模式：看 A/B 两路相位，决定加还是减

```text
编码器 A → TI1 ┐
                ├→ 编码器接口 → CNT 加1或减1
编码器 B → TI2 ┘
```

它不是单纯的“ETR 来一个脉冲加 1”，而是根据 A、B 相变化及方向计算增量，适合电机测速和方向识别。

### 3.5 为什么“输入捕获”不属于上面四种计数来源？

因为输入捕获在问的是**“外部边沿发生时 CNT 数到了哪里？”**，而不是**“用哪个信号推动 CNT 加 1？”**。

例如：

```text
内部 72 MHz → PSC → CNT 按 1 MHz 不断计时 ─────┐
                                                ├→ CCR1 锁存当时 CNT
外部方波 → PA6/TI1 → 上升沿捕获事件 ──────────┘
```

你可以一边让 CNT 按内部时钟跑，一边让外部边沿来“拍照”。再通过 TS/SMS 让该边沿复位 CNT，就能直接测一个波形周期。

**只要记住：CNT 计数来源和外部输入边沿的作用，不是一回事。**

---

## 4. 时基和事件：PSC、CNT、ARR 算时间，UEV/UIF 决定后续动作

### 4.1 STM32F103 定时器时钟从哪里来？

先要知道 `f_TIM` 是多少，不然 PSC 和 ARR 算得再对，实际时间也可能错。

F103 常见 72 MHz 时钟树：`SYSCLK=72 MHz`，`PCLK1=36 MHz`（APB1÷2），`PCLK2=72 MHz`（APB2÷1）。

- TIM2/3/4 在 APB1 上，**TIM 输入时钟为 72 MHz**，不是 36 MHz。
- TIM1 在 APB2 上，输入时钟也为 **72 MHz**。

原因是 F103 定时器时钟规则：**APB 预分频为1时，定时器时钟=PCLK；不为1时，定时器时钟=2×PCLK**。这里的 ×2 来自 RCC 时钟树，不是 PSC 自动乘2。不同工程时钟配置要重新核对。

![[assets/ppt/s062_RCC时钟树.jpg]]

### 4.2 PSC、CNT、ARR 分别干什么？

```text
输入时钟 f_TIM ─→ PSC 预分频 ─→ CNT 计数 ─→ ARR 决定周期末位置
```

| 名字 | 怎么看懂 | 例如 |
|---|---|---|
| **PSC** | 规定“每隔多少个输入时钟节拍，CNT 才加一次” | PSC=71，表示÷72 |
| **CNT** | 当前计数数值 | 0、1、2…… |
| **ARR** | 一轮计数的终点范围 | ARR=999，向上计数一轮有1000个计数节拍 |

内部时钟、向上计数、普通更新周期：

```text
f_CNT = f_TIM / (PSC + 1)
T_tick = 1 / f_CNT
T_update = (PSC + 1) × (ARR + 1) / f_TIM
```

例如 `f_TIM=72 MHz`、`PSC=71`、`ARR=9999`：

- 时钟先除以 72，得到 `1 MHz` CNT 计数频率。
- CNT 每 `1 μs` 加 1。
- 计数从 0 到 9999，一轮是 10000 个节拍。
- 一轮 `10 ms`，正常溢出更新周期就是 10 ms。

**为什么总要 +1？** PSC=71 代表分频 72；ARR=9999 代表从0开始数10000个节拍。

| 预分频时序 | 计数器更新时序 |
|---|---|
| ![[assets/ppt/s058_预分频器时序.jpg]] | ![[assets/ppt/s059_计数器时序.jpg]] |

### 4.3 CNT 除了向上数，还可以向下数或中心对齐

- **向上计数**：0→1→……→ARR→0。课程的大多数计时/PWM 代码用这个。
- **向下计数**：从 ARR 附近向 0 递减，然后重新开始。
- **中心对齐**：向上再向下计数，常用于某些电机 PWM。PWM 周期与普通向上计数的公式不同。

上面的 `ARR+1` 公式是**普通向上计数模式**；不要套在外部不规则计数、中心对齐或 TIM1 重复计数器上。

### 4.4 CKD 跟 PSC 不是同一个东西

在课程源码里有：

```c
TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
```

这里的 **CKD** 主要影响 `t_DTS`，关系到输入数字滤波器采样、部分高级定时器死区等内部时序。**不是把 CNT 的 72 MHz 分成 36 MHz 的那个 PSC。**

普通定时中断计算周期，重点仍是 TIM 时钟、PSC、ARR。

### 4.5 UEV、UIF、UIE、中断与 TRGO 不是同一个东西

先看最常见的向上计数：

```text
CNT 计数到周期末
       ↓
UEV：更新事件发生
       ├→ 更新预装载参数等内部工作
       ├→ UIF：在相应条件下置更新标志
       ├→ 若 UIE 及 NVIC 配置允许 → CPU 可进入 TIMx_IRQHandler()
       └→ 如果 MMS 选择 Update → 可以在 TRGO 输出更新触发
```

| 名字 | 大白话 | 重点 |
|---|---|---|
| **UEV** | 定时器内部发生了一次“更新” | 正常溢出、软件 UG 等都可能引起；具体受控制位影响 |
| **UIF** | 更新事件留下的“已发生”标志 | 不等于 CPU 一定进中断 |
| **UIE** | 更新中断允许位 | 没使能，中断请求不会因为 UIF 就自动发出 |
| **NVIC** | CPU 的中断入口管理 | 要配置通道使能和优先级 |
| **UG** | 软件生成更新事件的请求 | 初始化时可能导致 UIF 置位，启中断前常先清标志 |
| **TRGO** | 把选中的本 TIM 事件发给其他硬件 | 可不经过 CPU 中断直接触发 ADC 或其他定时器 |

**计数完一轮≠一定进入 ISR。** 即使没开更新中断，TIM 的计数和更新事件也可以照样发生。

### 4.6 主从触发：谁发、谁收，收到后做什么？

“主从”不是两个芯片的主板/从板，也不是 FreeRTOS 的主任务/从任务。这里仅指 TIM 之间或外设之间的硬件触发配合。

**发送一侧（主模式）**：用 `MMS` 选择 TIM 哪个事件作为 `TRGO`，例如 Update。

**接收一侧（从模式）**：用 `TS` 选择什么作为 `TRGI`，再用 `SMS` 选择收到信号之后该做什么：

- **Reset（复位）**：指定触发来到时，把 CNT 复位，方便测相邻边沿间隔。
- **Gated（门控）**：只有触发控制的门控条件有效时，计数器才运行。
- **Trigger（触发启动）**：指定触发到来时使计数器开始运行。
- **External Clock 1（外部时钟1）**：选中的 TRGI 有效事件推动 CNT 计数。

看这段输入捕获实验：

```c
TIM_SelectInputTrigger(TIM3, TIM_TS_TI1FP1);
TIM_SelectSlaveMode(TIM3, TIM_SlaveMode_Reset);
```

这里的 `TI1FP1` 是 **TRGI 的来源**，Reset 是**收到触发后的动作**。不要看到“TRGI”就以为它一定负责让 CNT 加 1。

### 4.7 TIM 触发 ADC 的实际例子（和上一章 ADC 连起来）

例如希望 ADC 每隔 10 ms 测一次电压，可以让支持的 TIM 触发源驱动 ADC，而不是让 CPU 每次进中断后再软件启动 ADC。

```text
TIM3 使用内部时钟计数
     ↓
PSC+ARR 配成每 10 ms 一次更新
     ↓
TIM3 MMS 选择 Update → TIM3_TRGO
     ↓
ADC1 选择 TIM3_TRGO 作为外部触发源（F103 支持的规则组触发之一）
     ↓
ADC 采样保持 + SAR 转换 → ADC_DR
     ↓
DMA 可把转换结果搬到内存
```

在这个场景下：

- **10 ms**：两次 ADC 启动之间的触发间隔。
- **几微秒**：ADC 一次采样和转换真正花的时间（由 ADC 时钟和 SampleTime 决定）。
- **TIM 的 TRGO**：只是“到时间了，开始采样”的通知；TIM 不负责把模拟电压变成数字。

---

## 5. 实验 6-1｜定时中断：每隔固定时间通知 CPU

### 5.1 工作过程

```text
RCC 提供 TIM2 时钟 → PSC 分频 → CNT 向上计数
→ CNT 周期结束 → 更新事件/置位标志 → 更新中断请求
→ NVIC 接受请求 → CPU 执行 TIM2_IRQHandler()
```

有了固定计数节拍，你可以让 TIM 每隔 1 ms、10 ms 或 1 s 请求一次中断。**中断函数应短小**；复杂计算、阻塞等待和 OLED 刷屏一般放主循环或任务中。

### 5.2 实验 6-1：TIM2 定时中断

课程假设 TIM2 时钟为 72 MHz：`PSC=7199` 得到 10 kHz 计数，`ARR=9999` 得到 **1 s 更新周期**。中断中 `Num++`，主循环显示计数。

**Timer.c — TIM2 定时器初始化**（原课程文件：`源码/6-1 定时器定时中断/System/Timer.c`）

```c
#include "stm32f10x.h"                  // Device header

/**
  * 函    数：定时中断初始化
  * 参    数：无
  * 返 回 值：无
  */
void Timer_Init(void)
{
	/*开启时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);			//开启TIM2的时钟
	
	/*配置时钟源*/
	TIM_InternalClockConfig(TIM2);		//选择TIM2为内部时钟，若不调用此函数，TIM默认也为内部时钟
	
	/*时基单元初始化*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;		//时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;	//计数器模式，选择向上计数
	TIM_TimeBaseInitStructure.TIM_Period = 10000 - 1;				//计数周期，即ARR的值
	TIM_TimeBaseInitStructure.TIM_Prescaler = 7200 - 1;				//预分频器，即PSC的值
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;			//重复计数器，高级定时器才会用到
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);				//将结构体变量交给TIM_TimeBaseInit，配置TIM2的时基单元	
	
	/*中断输出配置*/
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);						//清除定时器更新标志位
																//TIM_TimeBaseInit函数末尾，手动产生了更新事件
																//若不清除此标志位，则开启中断后，会立刻进入一次中断
																//如果不介意此问题，则不清除此标志位也可
	
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);					//开启TIM2的更新中断
	
	/*NVIC中断分组*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);				//配置NVIC为分组2
																//即抢占优先级范围：0~3，响应优先级范围：0~3
																//此分组配置在整个工程中仅需调用一次
																//若有多个中断，可以把此代码放在main函数内，while循环之前
																//若调用多次配置分组的代码，则后执行的配置会覆盖先执行的配置
	
	/*NVIC配置*/
	NVIC_InitTypeDef NVIC_InitStructure;						//定义结构体变量
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;				//选择配置NVIC的TIM2线
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//指定NVIC线路使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	//指定NVIC线路的抢占优先级为2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			//指定NVIC线路的响应优先级为1
	NVIC_Init(&NVIC_InitStructure);								//将结构体变量交给NVIC_Init，配置NVIC外设
	
	/*TIM使能*/
	TIM_Cmd(TIM2, ENABLE);			//使能TIM2，定时器开始运行
}

/* 定时器中断函数，可以复制到使用它的地方
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
*/
```

**main.c — 中断计数与 OLED 显示**（原课程文件：`源码/6-1 定时器定时中断/User/main.c`）

```c
#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Timer.h"

uint16_t Num;			//定义在定时器中断里自增的变量

int main(void)
{
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	Timer_Init();		//定时中断初始化
	
	/*显示静态字符串*/
	OLED_ShowString(1, 1, "Num:");			//1行1列显示字符串Num:
	
	while (1)
	{
		OLED_ShowNum(1, 5, Num, 5);			//不断刷新显示Num变量
	}
}

/**
  * 函    数：TIM2中断函数
  * 参    数：无
  * 返 回 值：无
  * 注意事项：此函数为中断函数，无需调用，中断触发后自动执行
  *           函数名为预留的指定名称，可以从启动文件复制
  *           请确保函数名正确，不能有任何差异，否则中断函数将不能进入
  */
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)		//判断是否是TIM2的更新事件触发的中断
	{
		Num ++;												//Num变量自增，用于测试定时中断
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);			//清除TIM2更新事件的中断标志位
															//中断标志位必须清除
															//否则中断将连续不断地触发，导致主程序卡死
	}
}
```

**理解源码时抓住：** `RCC_APB1PeriphClockCmd` 开时钟 → `TIM_TimeBaseInit` 配置 PSC/ARR → 清更新标志 → `TIM_ITConfig` 允许 TIM 更新中断 → NVIC 允许 CPU 响应 → `TIM_Cmd` 启动计数。

**工程注意：**

- `Num` 在中断中写、主循环中读，应至少声明为 `volatile uint16_t Num;`；`volatile` 解决编译器访问优化问题，不等于多任务同步或所有读写操作都原子。
- `NVIC_PriorityGroupConfig()` 全局只需合理配置一次，不要每个模块重复改分组。
- `TIM2_IRQHandler` 内需要检查并清除中断挂起标志，避免反复进入 ISR。
- `TIM_ClockDivision = TIM_CKD_DIV1` **不是** PSC 的 1 分频；它主要关系到数字滤波和死区等使用的时钟。


## 6. 实验 6-2｜外部时钟：每个有效脉冲让 CNT 计数

### 6.1 内部计时与外部计数有什么区别？

内部时钟模式的 CNT 按固定时钟速度递增，所以可以计时间；外部时钟模式按外部输入的有效脉冲递增，所以可以统计脉冲数。**外部脉冲到来的时间不一定等间隔**。

课程实验 6-2 使用 **TIM2 的 ETR 引脚（PA0）** 作为外部时钟输入，配置外部时钟模式 2：

```text
外部输入 ETR → 极性/分频/滤波 → PSC → CNT → 周期结束产生更新事件
```

若 `PSC=0` 且 `ARR=9`，理想情况下每 10 个**通过滤波的有效脉冲**产生一个正常更新周期。课程同时在 OLED 上显示 CNT 的当前值。

**不要把外部时钟的 ETR 和输入捕获的 CH1 混淆：** 两者都可能涉及 PA0/TIM2，但外部时钟让脉冲推动 CNT；输入捕获通常是 CNT 自己按内部时钟计数、在外部边沿到来时保存 CNT。

**对照第 3.3 节：** 这组代码选择的是 `ETR → ETRF → CK_PSC → PSC → CNT` 路线，不是走普通输入捕获。`TIM_ETRClockMode2Config()` 中的外部分频和滤波是给 ETR 输入的，和时基中的 `TIM_Prescaler` 是两层不同的配置。

**提醒：** 源码里写 `ETRFilter=0x0F` 并不是“越大越好”；它可能把合法的窄脉冲也滤掉。实际按输入频率和干扰情况设置。

**Timer.c — TIM2 外部时钟 ETR**（原课程文件：`源码/6-2 定时器外部时钟/System/Timer.c`）

```c
#include "stm32f10x.h"                  // Device header

/**
  * 函    数：定时中断初始化
  * 参    数：无
  * 返 回 值：无
  * 注意事项：此函数配置为外部时钟，定时器相当于计数器
  */
void Timer_Init(void)
{
	/*开启时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);			//开启TIM2的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);			//开启GPIOA的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);						//将PA0引脚初始化为上拉输入
	
	/*外部时钟配置*/
	TIM_ETRClockMode2Config(TIM2, TIM_ExtTRGPSC_OFF, TIM_ExtTRGPolarity_NonInverted, 0x0F);
																//选择外部时钟模式2，时钟从TIM_ETR引脚输入
																//注意TIM2的ETR引脚固定为PA0，无法随意更改
																//最后一个滤波器参数加到最大0x0F，可滤除时钟信号抖动
	
	/*时基单元初始化*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;		//时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;	//计数器模式，选择向上计数
	TIM_TimeBaseInitStructure.TIM_Period = 10 - 1;					//计数周期，即ARR的值
	TIM_TimeBaseInitStructure.TIM_Prescaler = 1 - 1;				//预分频器，即PSC的值
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;			//重复计数器，高级定时器才会用到
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);				//将结构体变量交给TIM_TimeBaseInit，配置TIM2的时基单元	
	
	/*中断输出配置*/
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);						//清除定时器更新标志位
																//TIM_TimeBaseInit函数末尾，手动产生了更新事件
																//若不清除此标志位，则开启中断后，会立刻进入一次中断
																//如果不介意此问题，则不清除此标志位也可
																
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);					//开启TIM2的更新中断
	
	/*NVIC中断分组*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);				//配置NVIC为分组2
																//即抢占优先级范围：0~3，响应优先级范围：0~3
																//此分组配置在整个工程中仅需调用一次
																//若有多个中断，可以把此代码放在main函数内，while循环之前
																//若调用多次配置分组的代码，则后执行的配置会覆盖先执行的配置
	
	/*NVIC配置*/
	NVIC_InitTypeDef NVIC_InitStructure;						//定义结构体变量
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;				//选择配置NVIC的TIM2线
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//指定NVIC线路使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	//指定NVIC线路的抢占优先级为2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			//指定NVIC线路的响应优先级为1
	NVIC_Init(&NVIC_InitStructure);								//将结构体变量交给NVIC_Init，配置NVIC外设
	
	/*TIM使能*/
	TIM_Cmd(TIM2, ENABLE);			//使能TIM2，定时器开始运行
}

/**
  * 函    数：返回定时器CNT的值
  * 参    数：无
  * 返 回 值：定时器CNT的值，范围：0~65535
  */
uint16_t Timer_GetCounter(void)
{
	return TIM_GetCounter(TIM2);	//返回定时器TIM2的CNT
}

/* 定时器中断函数，可以复制到使用它的地方
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
*/
```

**main.c — 显示 CNT 与更新次数**（原课程文件：`源码/6-2 定时器外部时钟/User/main.c`）

```c
#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Timer.h"

uint16_t Num;			//定义在定时器中断里自增的变量

int main(void)
{
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	Timer_Init();		//定时中断初始化
	
	/*显示静态字符串*/
	OLED_ShowString(1, 1, "Num:");			//1行1列显示字符串Num:
	OLED_ShowString(2, 1, "CNT:");			//2行1列显示字符串CNT:
	
	while (1)
	{
		OLED_ShowNum(1, 5, Num, 5);			//不断刷新显示Num变量
		OLED_ShowNum(2, 5, Timer_GetCounter(), 5);		//不断刷新显示CNT的值
	}
}

/**
  * 函    数：TIM2中断函数
  * 参    数：无
  * 返 回 值：无
  * 注意事项：此函数为中断函数，无需调用，中断触发后自动执行
  *           函数名为预留的指定名称，可以从启动文件复制
  *           请确保函数名正确，不能有任何差异，否则中断函数将不能进入
  */
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)		//判断是否是TIM2的更新事件触发的中断
	{
		Num ++;												//Num变量自增，用于测试定时中断
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);			//清除TIM2更新事件的中断标志位
															//中断标志位必须清除
															//否则中断将连续不断地触发，导致主程序卡死
	}
}
```

**工程注意：**

- 课程原注释中的“ETP”应写成 **ETR**（External Trigger，引脚/触发输入）；`ETP` 常用来指 ETR 极性控制位，不是 GPIO 引脚名称。
- `TIM2_ETR` 在这组默认引脚映射下是 **PA0**；实际芯片还需注意 AFIO 引脚复用/重映射及封装。
- `0x0F` 是课程使用的较强数字滤波设置，**不是越大越好**；有效脉冲太窄/太快可能被滤掉。滤波参数应按脉宽、噪声和滤波时钟选取。
- `Timer_GetCounter()` 注释写“范围 0～65535”只说明返回类型是 `uint16_t`；当前 `ARR=9` 时正常运行的 CNT 主要在 **0～9**。


## 7. 输出比较（OC）与 PWM：CNT 和 CCR 怎样控制输出？

### 7.1 CCR、OC、CH 分别是什么？

通用 TIM2/3/4 通常有 CH1～CH4 四个通道，对应 `CCR1～CCR4`（捕获/比较寄存器）。一个通道在某一时刻通常选择输入捕获或输出比较功能，不是同时拿同一路通道做所有功能。

| 名称 | 含义 | 在输出比较时的作用 |
|---|---|---|
| `CNT` | 当前计数值 | 不断向前计数 |
| `CCR1` 等 | 通道对应的捕获/比较寄存器 | 作为比较阈值 |
| `OC` | Output Compare，输出比较 | 判断 CNT 与 CCR 的关系，按模式产生动作 |
| `TIMx_CH1` 等 | 定时器通道相关的外部引脚 | 将 TIM 硬件产生的波形输出到引脚（需正确配置 GPIO） |

OC 不只是 PWM：还可在比较匹配时配置置位、清零、翻转等行为。**PWM 模式是输出比较的重要应用。**

| 图一 | 图二 |
|---|---|
| ![[assets/ppt/s063_输出比较简介.jpg]] | ![[assets/ppt/s067_输出比较模式.jpg]] |

| 图一 | 图二 |
|---|---|
| ![[assets/ppt/s066_输出比较通道(通用).jpg]] | ![[assets/ppt/s065_输出比较通道(高级).jpg]] |

### 7.2 PWM 为什么能控制占空比？

PWM（Pulse Width Modulation）是周期固定（或可调）的高低电平脉冲序列。常说两个量：

- **频率：** 一秒有多少个周期；`f_PWM = 1/T_PWM`。
- **占空比：** 一个周期里高电平所占的时间比例 `Duty = T_high/T_PWM`（以有效高电平为例）。

以向上计数、**PWM 模式 1、有效高电平**为例：当 `CNT < CCR` 时输出有效电平，达到 CCR 后变为无效电平；计数周期结束后重新开始。这样 CPU 不用不断 Set/Reset GPIO。

```text
f_PWM = f_TIM / [(PSC+1) × (ARR+1)]
Duty ≈ CCR / (ARR+1)    （向上计数，PWM1，有效高电平）
占空比最小调节步进约 = 1 / (ARR+1)
```

例如 `f_TIM=72 MHz`，`PSC=719`、`ARR=99`：

- 计数频率 100 kHz，每 10 μs 一个计数节拍。
- PWM 周期=100×10 μs=1 ms，即 **1 kHz**。
- `CCR=25`，高电平约 25%（250 μs）；`CCR=75`，约 75%。

**PSC 和 ARR 是一起决定 PWM 频率的**；在相同 PWM 频率下，PSC 越小通常可以让 ARR 越大，带来更细的 CCR 调节步进（但必须在定时器寄存器范围内，且受功能和分辨率限制）。

PWM1/PWM2、有效高/有效低、向上/向下/中心对齐的电平规律不完全相同，不能把上述占空比表达式无条件套给所有配置。

![[assets/ppt/s064_PWM简介.jpg]]

| 图一 | 图二 |
|---|---|
| ![[assets/ppt/s068_PWM基本结构.jpg]] | ![[assets/ppt/s069_参数计算.jpg]] |

### 7.3 GPIO、引脚复用和输出使能

定时器有内部 CH 通道，不代表随便一个 GPIO 都能输出该 TIM 的 PWM。要先查 **TIMx_CHy 的默认/重映射引脚**，然后：

1. 开 TIM 与 GPIO 时钟。
2. 按引脚映射配置输出引脚，例如标准库的 `GPIO_Mode_AF_PP`。
3. 配置 TIM 时基（PSC、ARR）及对应 `TIM_OCyInit()`（模式、极性、CCR/`Pulse`）。
4. 启用通道、启动 TIM；高级定时器 TIM1 还需要考虑主输出使能及死区/Break 等配置。

本笔记的课程例子：TIM2_CH1→PA0（呼吸灯）、TIM2_CH2→PA1（舵机）、TIM2_CH3→PA2（电机），以其默认映射为前提。


## 8. 实验 6-3～6-5｜用 PWM 控制灯、舵机和直流电机

### 8.1 实验 6-3：呼吸灯（CCR 往返变化）

原理：保持 PWM 频率基本不变，逐步改变 CCR，从而改变 LED 的平均驱动与视觉亮度。课程配置 TIM2_CH1（PA0），`PSC=719`、`ARR=99`，故 PWM 为 1 kHz。CCR 从 0 到 100 循环变化；对 PWM1 正常极性而言，这对应约 0%～100% 占空比。

**要理解：** `PWM_SetCompare1()` 改的是 CCR1，不是直接改变 CPU 的 GPIO 输出，也不是让 PWM 频率随亮度变化。

**PWM.c — TIM2_CH1 输出 PWM**（原课程文件：`源码/6-3 PWM驱动LED呼吸灯/Hardware/PWM.c`）

```c
#include "stm32f10x.h"                  // Device header

/**
  * 函    数：PWM初始化
  * 参    数：无
  * 返 回 值：无
  */
void PWM_Init(void)
{
	/*开启时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);			//开启TIM2的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);			//开启GPIOA的时钟
	
	/*GPIO重映射*/
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);			//开启AFIO的时钟，重映射必须先开启AFIO的时钟
//	GPIO_PinRemapConfig(GPIO_PartialRemap1_TIM2, ENABLE);			//将TIM2的引脚部分重映射，具体的映射方案需查看参考手册
//	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);		//将JTAG引脚失能，作为普通GPIO引脚使用
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;		//GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);							//将PA0引脚初始化为复用推挽输出	
																	//受外设控制的引脚，均需要配置为复用模式		
	
	/*配置时钟源*/
	TIM_InternalClockConfig(TIM2);		//选择TIM2为内部时钟，若不调用此函数，TIM默认也为内部时钟
	
	/*时基单元初始化*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     //时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; //计数器模式，选择向上计数
	TIM_TimeBaseInitStructure.TIM_Period = 100 - 1;					//计数周期，即ARR的值
	TIM_TimeBaseInitStructure.TIM_Prescaler = 720 - 1;				//预分频器，即PSC的值
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;            //重复计数器，高级定时器才会用到
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);             //将结构体变量交给TIM_TimeBaseInit，配置TIM2的时基单元
	
	/*输出比较初始化*/
	TIM_OCInitTypeDef TIM_OCInitStructure;							//定义结构体变量
	TIM_OCStructInit(&TIM_OCInitStructure);							//结构体初始化，若结构体没有完整赋值
																	//则最好执行此函数，给结构体所有成员都赋一个默认值
																	//避免结构体初值不确定的问题
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;				//输出比较模式，选择PWM模式1
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;		//输出极性，选择为高，若选择极性为低，则输出高低电平取反
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;	//输出使能
	TIM_OCInitStructure.TIM_Pulse = 0;								//初始的CCR值
	TIM_OC1Init(TIM2, &TIM_OCInitStructure);						//将结构体变量交给TIM_OC1Init，配置TIM2的输出比较通道1
	
	/*TIM使能*/
	TIM_Cmd(TIM2, ENABLE);			//使能TIM2，定时器开始运行
}

/**
  * 函    数：PWM设置CCR
  * 参    数：Compare 要写入的CCR的值，范围：0~100
  * 返 回 值：无
  * 注意事项：CCR和ARR共同决定占空比，此函数仅设置CCR的值，并不直接是占空比
  *           占空比Duty = CCR / (ARR + 1)
  */
void PWM_SetCompare1(uint16_t Compare)
{
	TIM_SetCompare1(TIM2, Compare);		//设置CCR1的值
}
```

**main.c — 逐步改变占空比**（原课程文件：`源码/6-3 PWM驱动LED呼吸灯/User/main.c`）

```c
#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "PWM.h"

uint8_t i;			//定义for循环的变量

int main(void)
{
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	PWM_Init();			//PWM初始化
	
	while (1)
	{
		for (i = 0; i <= 100; i++)
		{
			PWM_SetCompare1(i);			//依次将定时器的CCR寄存器设置为0~100，PWM占空比逐渐增大，LED逐渐变亮
			Delay_ms(10);				//延时10ms
		}
		for (i = 0; i <= 100; i++)
		{
			PWM_SetCompare1(100 - i);	//依次将定时器的CCR寄存器设置为100~0，PWM占空比逐渐减小，LED逐渐变暗
			Delay_ms(10);				//延时10ms
		}
	}
}
```

**工程注意：** LED 是低电平点亮还是高电平点亮，取决于接法，实际“CCR 越大越亮”不能脱离电路判断；课程示例默认驱动关系如此。正式使用时可用定时器预装载避免修改 CCR 后产生不合适的周期内毛刺。

### 8.2 实验 6-4：舵机（占空比不是直接等于角度）

课程所用舵机使用**约 20 ms 的控制脉冲周期**，用**高电平脉宽**表示目标角度，而不是单纯看占空比百分数。

TIM2_CH2→PA1，`PSC=71`，CNT 每 1 μs 加 1；`ARR=19999`，周期 20000 μs=20 ms=50 Hz。

课程 `Servo_SetAngle()` 约定：角度 0～180° 映射到 `CCR=500～2500`，也就是 0.5～2.5 ms 的高电平脉宽。**该脉宽范围只表示课程使用的舵机设定，不是所有舵机统一标准；接具体舵机时要查规格。**

| 图一 | 图二 |
|---|---|
| ![[assets/ppt/s070_舵机简介.jpg]] | ![[assets/ppt/s071_硬件电路.jpg]] |

**PWM.c — TIM2_CH2 产生 50 Hz PWM**（原课程文件：`源码/6-4 PWM驱动舵机/Hardware/PWM.c`）

```c
#include "stm32f10x.h"                  // Device header

/**
  * 函    数：PWM初始化
  * 参    数：无
  * 返 回 值：无
  */
void PWM_Init(void)
{
	/*开启时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);			//开启TIM2的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);			//开启GPIOA的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);							//将PA1引脚初始化为复用推挽输出	
																	//受外设控制的引脚，均需要配置为复用模式
	
	/*配置时钟源*/
	TIM_InternalClockConfig(TIM2);		//选择TIM2为内部时钟，若不调用此函数，TIM默认也为内部时钟
	
	/*时基单元初始化*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     //时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; //计数器模式，选择向上计数
	TIM_TimeBaseInitStructure.TIM_Period = 20000 - 1;				//计数周期，即ARR的值
	TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;				//预分频器，即PSC的值
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;            //重复计数器，高级定时器才会用到
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);             //将结构体变量交给TIM_TimeBaseInit，配置TIM2的时基单元
	
	/*输出比较初始化*/ 
	TIM_OCInitTypeDef TIM_OCInitStructure;							//定义结构体变量
	TIM_OCStructInit(&TIM_OCInitStructure);                         //结构体初始化，若结构体没有完整赋值
	                                                                //则最好执行此函数，给结构体所有成员都赋一个默认值
	                                                                //避免结构体初值不确定的问题
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;               //输出比较模式，选择PWM模式1
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;       //输出极性，选择为高，若选择极性为低，则输出高低电平取反
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;   //输出使能
	TIM_OCInitStructure.TIM_Pulse = 0;								//初始的CCR值
	TIM_OC2Init(TIM2, &TIM_OCInitStructure);                        //将结构体变量交给TIM_OC2Init，配置TIM2的输出比较通道2
	
	/*TIM使能*/
	TIM_Cmd(TIM2, ENABLE);			//使能TIM2，定时器开始运行
}

/**
  * 函    数：PWM设置CCR
  * 参    数：Compare 要写入的CCR的值，范围：0~100
  * 返 回 值：无
  * 注意事项：CCR和ARR共同决定占空比，此函数仅设置CCR的值，并不直接是占空比
  *           占空比Duty = CCR / (ARR + 1)
  */
void PWM_SetCompare2(uint16_t Compare)
{
	TIM_SetCompare2(TIM2, Compare);		//设置CCR2的值
}
```

**Servo.c — 将角度映射到脉宽**（原课程文件：`源码/6-4 PWM驱动舵机/Hardware/Servo.c`）

```c
#include "stm32f10x.h"                  // Device header
#include "PWM.h"

/**
  * 函    数：舵机初始化
  * 参    数：无
  * 返 回 值：无
  */
void Servo_Init(void)
{
	PWM_Init();									//初始化舵机的底层PWM
}

/**
  * 函    数：舵机设置角度
  * 参    数：Angle 要设置的舵机角度，范围：0~180
  * 返 回 值：无
  */
void Servo_SetAngle(float Angle)
{
	PWM_SetCompare2(Angle / 180 * 2000 + 500);	//设置占空比
												//将角度线性变换，对应到舵机要求的占空比范围上
}
```

**main.c — 按键调整目标角度**（原课程文件：`源码/6-4 PWM驱动舵机/User/main.c`）

```c
#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Servo.h"
#include "Key.h"

uint8_t KeyNum;			//定义用于接收键码的变量
float Angle;			//定义角度变量

int main(void)
{
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	Servo_Init();		//舵机初始化
	Key_Init();			//按键初始化
	
	/*显示静态字符串*/
	OLED_ShowString(1, 1, "Angle:");	//1行1列显示字符串Angle:
	
	while (1)
	{
		KeyNum = Key_GetNum();			//获取按键键码
		if (KeyNum == 1)				//按键1按下
		{
			Angle += 30;				//角度变量自增30
			if (Angle > 180)			//角度变量超过180后
			{
				Angle = 0;				//角度变量归零
			}
		}
		Servo_SetAngle(Angle);			//设置舵机的角度为角度变量
		OLED_ShowNum(1, 7, Angle, 3);	//OLED显示角度变量
	}
}
```

**源码理解/修正：**

- `PWM_SetCompare2(Angle / 180 * 2000 + 500)` 改的是**以 1 μs 计数为单位的高电平时长**：500～2500 μs，不是把角度直接换成一个 0～100% 占空比参数。
- 原 `PWM_SetCompare2()` 的注释把 `Compare` 范围写为“0～100”，**与实际舵机调用的 500～2500 不一致**，需要改为符合 16 位定时器/本实验约定的范围。
- `Servo_SetAngle()` 没有限制输入角度，工程中应按舵机允许角度和脉宽范围限幅；舵机供电与 MCU GPIO 信号电平/共地也必须满足实际模块要求。

### 8.3 实验 6-5：直流电机（方向由驱动器决定，PWM 调平均驱动）

MCU GPIO 不应直接给电机供电：STM32 输出方向控制信号和 PWM，**电机驱动芯片**负责将外部电源的电能送给电机。

课程将 TIM2_CH3→PA2 用作 PWM，PA4/PA5 用来控制驱动器方向。`PSC=35`、`ARR=99`，PWM 频率为 `72 MHz/(36×100)=20 kHz`。

```text
Motor_SetSpeed(+60)：方向 GPIO 选择正转，CCR3=60
Motor_SetSpeed(-60)：方向 GPIO 选择反转，CCR3=60
```

**特别注意：** 函数参数 `Speed` 在这个教学例子里其实是**有符号 PWM 控制量（百分比近似）**，并不是真实 RPM，也不是闭环速度控制。真实转速还会受电机负载、电源、电机驱动器等影响。

| 图一 | 图二 |
|---|---|
| ![[assets/ppt/s072_直流电机及驱动简介.jpg]] | ![[assets/ppt/s073_硬件电路.jpg]] |

**PWM.c — TIM2_CH3 输出电机 PWM**（原课程文件：`源码/6-5 PWM驱动直流电机/Hardware/PWM.c`）

```c
#include "stm32f10x.h"                  // Device header

/**
  * 函    数：PWM初始化
  * 参    数：无
  * 返 回 值：无
  */
void PWM_Init(void)
{
	/*开启时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);			//开启TIM2的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);			//开启GPIOA的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);							//将PA2引脚初始化为复用推挽输出	
																	//受外设控制的引脚，均需要配置为复用模式
	
	/*配置时钟源*/
	TIM_InternalClockConfig(TIM2);		//选择TIM2为内部时钟，若不调用此函数，TIM默认也为内部时钟
	
	/*时基单元初始化*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     //时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; //计数器模式，选择向上计数
	TIM_TimeBaseInitStructure.TIM_Period = 100 - 1;                 //计数周期，即ARR的值
	TIM_TimeBaseInitStructure.TIM_Prescaler = 36 - 1;               //预分频器，即PSC的值
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;            //重复计数器，高级定时器才会用到
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);             //将结构体变量交给TIM_TimeBaseInit，配置TIM2的时基单元
	
	/*输出比较初始化*/ 
	TIM_OCInitTypeDef TIM_OCInitStructure;							//定义结构体变量
	TIM_OCStructInit(&TIM_OCInitStructure);                         //结构体初始化，若结构体没有完整赋值
	                                                                //则最好执行此函数，给结构体所有成员都赋一个默认值
	                                                                //避免结构体初值不确定的问题
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;               //输出比较模式，选择PWM模式1
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;       //输出极性，选择为高，若选择极性为低，则输出高低电平取反
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;   //输出使能
	TIM_OCInitStructure.TIM_Pulse = 0;								//初始的CCR值
	TIM_OC3Init(TIM2, &TIM_OCInitStructure);                        //将结构体变量交给TIM_OC3Init，配置TIM2的输出比较通道3
	
	/*TIM使能*/
	TIM_Cmd(TIM2, ENABLE);			//使能TIM2，定时器开始运行
}

/**
  * 函    数：PWM设置CCR
  * 参    数：Compare 要写入的CCR的值，范围：0~100
  * 返 回 值：无
  * 注意事项：CCR和ARR共同决定占空比，此函数仅设置CCR的值，并不直接是占空比
  *           占空比Duty = CCR / (ARR + 1)
  */
void PWM_SetCompare3(uint16_t Compare)
{
	TIM_SetCompare3(TIM2, Compare);		//设置CCR3的值
}
```

**Motor.c — 方向与占空比控制**（原课程文件：`源码/6-5 PWM驱动直流电机/Hardware/Motor.c`）

```c
#include "stm32f10x.h"                  // Device header
#include "PWM.h"

/**
  * 函    数：直流电机初始化
  * 参    数：无
  * 返 回 值：无
  */
void Motor_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启GPIOA的时钟
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);						//将PA4和PA5引脚初始化为推挽输出	
	
	PWM_Init();													//初始化直流电机的底层PWM
}

/**
  * 函    数：直流电机设置速度
  * 参    数：Speed 要设置的速度，范围：-100~100
  * 返 回 值：无
  */
void Motor_SetSpeed(int8_t Speed)
{
	if (Speed >= 0)							//如果设置正转的速度值
	{
		GPIO_SetBits(GPIOA, GPIO_Pin_4);	//PA4置高电平
		GPIO_ResetBits(GPIOA, GPIO_Pin_5);	//PA5置低电平，设置方向为正转
		PWM_SetCompare3(Speed);				//PWM设置为速度值
	}
	else									//否则，即设置反转的速度值
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_4);	//PA4置低电平
		GPIO_SetBits(GPIOA, GPIO_Pin_5);	//PA5置高电平，设置方向为反转
		PWM_SetCompare3(-Speed);			//PWM设置为负的速度值，因为此时速度值为负数，而PWM只能给正数
	}
}
```

**main.c — 按键改变方向与 PWM**（原课程文件：`源码/6-5 PWM驱动直流电机/User/main.c`）

```c
#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Motor.h"
#include "Key.h"

uint8_t KeyNum;		//定义用于接收按键键码的变量
int8_t Speed;		//定义速度变量

int main(void)
{
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	Motor_Init();		//直流电机初始化
	Key_Init();			//按键初始化
	
	/*显示静态字符串*/
	OLED_ShowString(1, 1, "Speed:");		//1行1列显示字符串Speed:
	
	while (1)
	{
		KeyNum = Key_GetNum();				//获取按键键码
		if (KeyNum == 1)					//按键1按下
		{
			Speed += 20;					//速度变量自增20
			if (Speed > 100)				//速度变量超过100后
			{
				Speed = -100;				//速度变量变为-100
											//此操作会让电机旋转方向突然改变，可能会因供电不足而导致单片机复位
											//若出现了此现象，则应避免使用这样的操作
			}
		}
		Motor_SetSpeed(Speed);				//设置直流电机的速度为速度变量
		OLED_ShowSignedNum(1, 7, Speed, 3);	//OLED显示速度变量
	}
}
```

**工程注意：**

- 原程序从正转大占空比直接切换到反转大占空比，可能带来明显电流冲击、机械冲击甚至 MCU 掉电复位。正式电机控制应设计**限幅、目标斜坡、换向策略与过流保护**。
- 本例 `Speed=-100～100` 是教学控制参数；你实际双 MCU 电机项目的目标 RPM、测量 RPM、PI 输出和电机方向，需要在各软件层保持一致，不能把占空比直接写成 RPM。
- `TIM2_CH3`、方向 GPIO 的有效电平和电机驱动器接法必须结合原理图确定。


## 9. 实验 6-6｜输入捕获（IC）：测量外部波形的时间

### 9.1 输出比较与输入捕获正好相反

- **输出比较 OC：** CNT 不断计数，硬件拿 CNT 去与 CCR 比较，决定何时输出动作。
- **输入捕获 IC：** CNT 通常按内部时钟计数；指定输入边沿到来时，硬件将 CNT 当前值**锁存到 CCR**，供软件读取。

假设 CNT 频率 1 MHz，则每计数一次经过 1 μs；两个上升沿间隔 1000 个计数节拍，信号周期约 1 ms，频率约 1 kHz。

```text
T_signal = 捕获间隔计数 / f_CNT
f_signal = f_CNT / 捕获间隔计数
```

既可以连续捕获两次并求差，也可以配置定时器**从模式 Reset**，由选定的输入边沿自动清零 CNT，这样后续捕获值代表上一周期的计数长度。

**不要混淆：** 输入信号经过 TI1/CH1 引脚进入，CCR1 是**硬件捕获结果寄存器**；不是“外部引脚提供了 1 MHz 时钟”。

![[assets/ppt/s074_输入捕获简介.jpg]]

| 图一 | 图二 |
|---|---|
| ![[assets/ppt/s076_输入捕获通道.jpg]] | ![[assets/ppt/s077_主从触发模式.jpg]] |

![[assets/ppt/s078_输入捕获基本结构.jpg]]

### 9.2 测频法和测周法的选择

- **测频法：** 固定门控时间（如 1 s）内数多少个外部脉冲；高频时相对计数误差通常更小。
- **测周法：** 用高速内部计数器测相邻有效边沿的时间差；低频时通常更合适，但很长周期可能发生 CNT 溢出。

选择要结合目标频率范围、计数器位宽、边沿抖动、更新速度与误差要求，不是“输入捕获永远比外部计数准”。

![[assets/ppt/s075_频率测量.jpg]]

### 9.3 实验 6-6：TIM3 输入捕获测频率

课程：TIM3_CH1→PA6，内部 CNT 频率 1 MHz，输入配置为上升沿捕获，并让 `TI1FP1` 作为复位触发源。每来一个有效上升沿，硬件锁存 CCR1 并复位 CNT。主循环读取 CCR1 计算输入频率。

**把第 2 章的词连起来看：** PA6/TI1 是输入引脚和输入路径；经过处理形成的 `TI1FP1` 可以被 `TS` 选为 `TRGI`；`SMS=Reset` 让边沿到来时 CNT 清零。**CNT 在边沿之间依然由内部时钟推进**，所以 CCR1 中保存的数字才能换算成时间。

**源码注意：** `IC_GetFreq()` 中的 `+1` 是课程沿用的写法，测周期时不能把“ARR+1”的规则机械套到捕获计数。真实计算要按锁存的捕获时序和误差要求判断；还需处理零值、超时无边沿、定时器溢出。

**IC.c — TIM3 输入捕获与 Reset 从模式**（原课程文件：`源码/6-6 输入捕获模式测频率/Hardware/IC.c`）

```c
#include "stm32f10x.h"                  // Device header

/**
  * 函    数：输入捕获初始化
  * 参    数：无
  * 返 回 值：无
  */
void IC_Init(void)
{
	/*开启时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);			//开启TIM3的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);			//开启GPIOA的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);							//将PA6引脚初始化为上拉输入
	
	/*配置时钟源*/
	TIM_InternalClockConfig(TIM3);		//选择TIM3为内部时钟，若不调用此函数，TIM默认也为内部时钟
	
	/*时基单元初始化*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     //时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; //计数器模式，选择向上计数
	TIM_TimeBaseInitStructure.TIM_Period = 65536 - 1;               //计数周期，即ARR的值
	TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;               //预分频器，即PSC的值
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;            //重复计数器，高级定时器才会用到
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);             //将结构体变量交给TIM_TimeBaseInit，配置TIM3的时基单元
	
	/*输入捕获初始化*/
	TIM_ICInitTypeDef TIM_ICInitStructure;							//定义结构体变量
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;				//选择配置定时器通道1
	TIM_ICInitStructure.TIM_ICFilter = 0xF;							//输入滤波器参数，可以过滤信号抖动
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;		//极性，选择为上升沿触发捕获
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;			//捕获预分频，选择不分频，每次信号都触发捕获
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;	//输入信号交叉，选择直通，不交叉
	TIM_ICInit(TIM3, &TIM_ICInitStructure);							//将结构体变量交给TIM_ICInit，配置TIM3的输入捕获通道
	
	/*选择触发源及从模式*/
	TIM_SelectInputTrigger(TIM3, TIM_TS_TI1FP1);					//触发源选择TI1FP1
	TIM_SelectSlaveMode(TIM3, TIM_SlaveMode_Reset);					//从模式选择复位
																	//即TI1产生上升沿时，会触发CNT归零
	
	/*TIM使能*/
	TIM_Cmd(TIM3, ENABLE);			//使能TIM3，定时器开始运行
}

/**
  * 函    数：获取输入捕获的频率
  * 参    数：无
  * 返 回 值：捕获得到的频率
  */
uint32_t IC_GetFreq(void)
{
	return 1000000 / (TIM_GetCapture1(TIM3) + 1);		//测周法得到频率fx = fc / N，这里不执行+1的操作也可
}
```

**main.c — 用 PWM 产生测试信号并显示频率**（原课程文件：`源码/6-6 输入捕获模式测频率/User/main.c`）

```c
#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "PWM.h"
#include "IC.h"

int main(void)
{
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	PWM_Init();			//PWM初始化
	IC_Init();			//输入捕获初始化
	
	/*显示静态字符串*/
	OLED_ShowString(1, 1, "Freq:00000Hz");		//1行1列显示字符串Freq:00000Hz
	
	/*使用PWM模块提供输入捕获的测试信号*/
	PWM_SetPrescaler(720 - 1);					//PWM频率Freq = 72M / (PSC + 1) / 100
	PWM_SetCompare1(50);						//PWM占空比Duty = CCR / 100
	
	while (1)
	{
		OLED_ShowNum(1, 6, IC_GetFreq(), 5);	//不断刷新显示输入捕获测得的频率
	}
}
```

**源码理解/修正：**

- 原函数 `1000000/(TIM_GetCapture1(TIM3)+1)` 在教学代码里有 `+1`，**不能像 ARR+1 那样机械套用**。周期捕获值应按实际定时器捕获/复位时序理解，通常用 `f_CNT/周期计数`；在特定配置下可能涉及一计数误差补偿。应核对 RM0008 时序及实际波形，避免无理由加 1。
- 没有有效捕获、捕获值为 0 时不能直接做除法；长期无新边沿时，CCR 可能一直保存旧值，软件还需要处理**信号超时**。
- TIM3 16 位、CNT 1 MHz 时，约 65.536 ms 就会回绕。对于周期超过此范围的信号，这份课程示例**不具备可靠的多圈溢出处理**。
- `TIM_ICFilter=0xF` 不是总能用；输入频率较高或脉宽较短时，强滤波可能抹掉真实边沿。


## 10. 实验 6-7｜PWMI：同时测方波频率和占空比

普通输入捕获重点是边沿到来时存一次 CNT；PWM 输入模式（PWMI）是输入捕获的组合配置：**一根外部 PWM 输入信号，通过两个捕获通道分别记录周期和高电平宽度**。

以 TIM3 的 TI1（PA6）输入为例：

1. IC1 记录相邻上升沿间的周期计数，结果存 **CCR1**。
2. IC2 记录上升沿到下降沿的高电平计数，结果存 **CCR2**。
3. 配置 TI1FP1 触发 Reset 从模式，使上升沿成为计数起点。

```text
输入频率 ≈ f_CNT / CCR1
占空比 ≈ CCR2 / CCR1 × 100%
```

这个是本课程上升沿复位、有效高电平 PWMI 的正常计算思路；具体寄存器捕获时序、信号是否稳定和边沿极性都必须正确。只有一个外部 PWM 输入引脚，却可能使用两个内部捕获通道，**不是两根信号线都要接到外部**。

![[assets/ppt/s079_PWMI基本结构.jpg]]

### 10.1 实验 6-7：PWMI 模式测量频率和占空比

**为什么只接一根 PA6 却能测两个量？** 因为输入捕获电路可以把同一路 TI1 信号映射到两个内部捕获通道：一个在上升沿记录周期，另一个在下降沿记录高电平时间。不是再找一根 PA7 线接同一个 PWM。

**源码注意：** 原课程用 `(CCR+1)` 近似计算，理解原理时先记 `频率≈f_CNT/周期捕获值`、`占空比≈高电平捕获值/周期捕获值`；实际还要避免除零、处理信号丢失与跨周期更新。

**IC.c — 同时读取 CCR1、CCR2**（原课程文件：`源码/6-7 PWMI模式测频率占空比/Hardware/IC.c`）

```c
#include "stm32f10x.h"                  // Device header

/**
  * 函    数：输入捕获初始化
  * 参    数：无
  * 返 回 值：无
  */
void IC_Init(void)
{
	/*开启时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);			//开启TIM3的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);			//开启GPIOA的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);							//将PA6引脚初始化为上拉输入
	
	/*配置时钟源*/
	TIM_InternalClockConfig(TIM3);		//选择TIM3为内部时钟，若不调用此函数，TIM默认也为内部时钟
	
	/*时基单元初始化*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     //时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; //计数器模式，选择向上计数
	TIM_TimeBaseInitStructure.TIM_Period = 65536 - 1;               //计数周期，即ARR的值
	TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;               //预分频器，即PSC的值
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;            //重复计数器，高级定时器才会用到
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);             //将结构体变量交给TIM_TimeBaseInit，配置TIM3的时基单元
	
	/*PWMI模式初始化*/
	TIM_ICInitTypeDef TIM_ICInitStructure;							//定义结构体变量
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;				//选择配置定时器通道1
	TIM_ICInitStructure.TIM_ICFilter = 0xF;							//输入滤波器参数，可以过滤信号抖动
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;		//极性，选择为上升沿触发捕获
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;			//捕获预分频，选择不分频，每次信号都触发捕获
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;	//输入信号交叉，选择直通，不交叉
	TIM_PWMIConfig(TIM3, &TIM_ICInitStructure);						//将结构体变量交给TIM_PWMIConfig，配置TIM3的输入捕获通道
																	//此函数同时会把另一个通道配置为相反的配置，实现PWMI模式

	/*选择触发源及从模式*/
	TIM_SelectInputTrigger(TIM3, TIM_TS_TI1FP1);					//触发源选择TI1FP1
	TIM_SelectSlaveMode(TIM3, TIM_SlaveMode_Reset);					//从模式选择复位
																	//即TI1产生上升沿时，会触发CNT归零
	
	/*TIM使能*/
	TIM_Cmd(TIM3, ENABLE);			//使能TIM3，定时器开始运行
}

/**
  * 函    数：获取输入捕获的频率
  * 参    数：无
  * 返 回 值：捕获得到的频率
  */
uint32_t IC_GetFreq(void)
{
	return 1000000 / (TIM_GetCapture1(TIM3) + 1);		//测周法得到频率fx = fc / N，这里不执行+1的操作也可
}

/**
  * 函    数：获取输入捕获的占空比
  * 参    数：无
  * 返 回 值：捕获得到的占空比
  */
uint32_t IC_GetDuty(void)
{
	return (TIM_GetCapture2(TIM3) + 1) * 100 / (TIM_GetCapture1(TIM3) + 1);	//占空比Duty = CCR2 / CCR1 * 100，这里不执行+1的操作也可
}
```

**main.c — 同时显示频率与占空比**（原课程文件：`源码/6-7 PWMI模式测频率占空比/User/main.c`）

```c
#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "PWM.h"
#include "IC.h"

int main(void)
{
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	PWM_Init();			//PWM初始化
	IC_Init();			//输入捕获初始化
	
	/*显示静态字符串*/
	OLED_ShowString(1, 1, "Freq:00000Hz");		//1行1列显示字符串Freq:00000Hz
	OLED_ShowString(2, 1, "Duty:00%");			//2行1列显示字符串Duty:00%
	
	/*使用PWM模块提供输入捕获的测试信号*/
	PWM_SetPrescaler(720 - 1);					//PWM频率Freq = 72M / (PSC + 1) / 100
	PWM_SetCompare1(50);						//PWM占空比Duty = CCR / 100
	
	while (1)
	{
		OLED_ShowNum(1, 6, IC_GetFreq(), 5);	//不断刷新显示输入捕获测得的频率
		OLED_ShowNum(2, 6, IC_GetDuty(), 2);	//不断刷新显示输入捕获测得的占空比
	}
}
```

**工程注意：** 原代码在 `CCR1/CCR2` 后都加了 `+1`，不应未经时序验证就当成固定定理；应保证 `CCR1≠0`，并处理无信号超时、脉冲宽度越界、捕获溢出，以及 CCR1/CCR2 **不同周期更新时的读数一致性**。另外 `TIM_PWMIConfig()` 使用的是一组输入捕获配置，同时配置配对的另一路捕获。


## 11. 实验 6-8｜编码器：TIM 如何得到方向和计数增量？

### 11.1 为什么需要 A/B 两相信号？

增量式正交编码器通常输出 A、B 两路相位相差约 90° 的方波。A/B 的先后顺序能判断转动方向；定时器根据设定的编码器模式与极性，遇到有效边沿时使 **CNT 增加或减少**。

```text
普通定时：    时钟节拍 → CNT 加 1
编码器模式：  A/B 有效边沿 + 方向判断 → CNT 加 1 或减 1
```

与用 EXTI 给每一个编码器边沿都触发 CPU 中断相比，TIM 编码器接口能由硬件完成方向判断与计数；CPU 只需按照控制周期读取 CNT。

| 图一 | 图二 |
|---|---|
| ![[assets/ppt/s080_编码器接口简介.jpg]] | ![[assets/ppt/s081_正交编码器.jpg]] |

| 图一 | 图二 |
|---|---|
| ![[assets/ppt/s082_编码器接口基本结构.jpg]] | ![[assets/ppt/s083_工作模式.jpg]] |

| 图一 | 图二 |
|---|---|
| ![[assets/ppt/s084_实例（均不反相）.jpg]] | ![[assets/ppt/s085_实例（TI1反相）.jpg]] |

### 11.2 编码器计数模式与极性

TIM 编码器模式可以选择主要在 TI1、TI2 或两者上计数；常见的 x1/x2/x4 说法是相对于编码器单路每周期的计数倍率，具体还要看外设选择和编码器厂商对“线数/PPR/CPR”的定义。

**不要直接把“编码器 1000 PPR”当成 MCU 每圈一定有 1000 个 CNT 增量。** 使用双边沿和双通道计数时，实际计数数可能不同，需要先确认倍频方式、减速比以及编码器装在哪根轴上。

原课程配置 TIM3 的 TI1=PA6、TI2=PA7，并使用 `TIM_EncoderMode_TI12`。代码中的 `TIM_ICPolarity_Rising` 主要决定信号是否反相；不能简单解释成“只检测上升沿，下降沿都不计数”。

### 11.3 固定时间窗口内怎样计算转速？

设每间隔 `T_s` 秒读一次编码器增量 `ΔCNT`；被测轴每转对应 `N` 个实际计数，则：

```text
RPM = ΔCNT × 60 / (N × T_s)
```

例如 `ΔCNT=80`，`N=800 counts/rev`，`T_s=0.01 s`，那么 `RPM=600`。增量为负时，RPM 也应保持负号，表示方向。

**课程里 `Speed=Encoder_Get()` 不等于 RPM。** 它只是固定窗口内的带符号增量计数。要得到 RPM，还要结合采样周期和编码器每圈实际计数数换算。

### 11.4 实验 6-8：TIM3 编码器 + TIM2 周期读取

TIM3 使用编码器模式记录方向/脉冲，TIM2 每隔 1 s 进一次中断，读取 TIM3 当前计数并清零，再将增量显示在 OLED。课程实验有意保留原实现，便于对照教学。

**对照第 3.4 节：** TIM3 的 TI1/TI2（默认 PA6/PA7）用于判断方向和产生编码器计数。`TIM_EncoderMode_TI12` 让 TIM3 在两路信号上按所选模式计数。此时 `CNT` 不表示几微秒，而是位置计数的模值。

**Encoder.c — TIM3 编码器接口**（原课程文件：`源码/6-8 编码器接口测速/Hardware/Encoder.c`）

```c
#include "stm32f10x.h"                  // Device header

/**
  * 函    数：编码器初始化
  * 参    数：无
  * 返 回 值：无
  */
void Encoder_Init(void)
{
	/*开启时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);			//开启TIM3的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);			//开启GPIOA的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);							//将PA6和PA7引脚初始化为上拉输入
	
	/*时基单元初始化*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     //时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; //计数器模式，选择向上计数
	TIM_TimeBaseInitStructure.TIM_Period = 65536 - 1;               //计数周期，即ARR的值
	TIM_TimeBaseInitStructure.TIM_Prescaler = 1 - 1;                //预分频器，即PSC的值
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;            //重复计数器，高级定时器才会用到
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);             //将结构体变量交给TIM_TimeBaseInit，配置TIM3的时基单元
	
	/*输入捕获初始化*/
	TIM_ICInitTypeDef TIM_ICInitStructure;							//定义结构体变量
	TIM_ICStructInit(&TIM_ICInitStructure);							//结构体初始化，若结构体没有完整赋值
																	//则最好执行此函数，给结构体所有成员都赋一个默认值
																	//避免结构体初值不确定的问题
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;				//选择配置定时器通道1
	TIM_ICInitStructure.TIM_ICFilter = 0xF;							//输入滤波器参数，可以过滤信号抖动
	TIM_ICInit(TIM3, &TIM_ICInitStructure);							//将结构体变量交给TIM_ICInit，配置TIM3的输入捕获通道
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;				//选择配置定时器通道2
	TIM_ICInitStructure.TIM_ICFilter = 0xF;							//输入滤波器参数，可以过滤信号抖动
	TIM_ICInit(TIM3, &TIM_ICInitStructure);							//将结构体变量交给TIM_ICInit，配置TIM3的输入捕获通道
	
	/*编码器接口配置*/
	TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
																	//配置编码器模式以及两个输入通道是否反相
																	//注意此时参数的Rising和Falling已经不代表上升沿和下降沿了，而是代表是否反相
																	//此函数必须在输入捕获初始化之后进行，否则输入捕获的配置会覆盖此函数的部分配置
	
	/*TIM使能*/
	TIM_Cmd(TIM3, ENABLE);			//使能TIM3，定时器开始运行
}

/**
  * 函    数：获取编码器的增量值
  * 参    数：无
  * 返 回 值：自上此调用此函数后，编码器的增量值
  */
int16_t Encoder_Get(void)
{
	/*使用Temp变量作为中继，目的是返回CNT后将其清零*/
	int16_t Temp;
	Temp = TIM_GetCounter(TIM3);
	TIM_SetCounter(TIM3, 0);
	return Temp;
}
```

**Timer.c — TIM2 产生测速窗口**（原课程文件：`源码/6-8 编码器接口测速/System/Timer.c`）

```c
#include "stm32f10x.h"                  // Device header

/**
  * 函    数：定时中断初始化
  * 参    数：无
  * 返 回 值：无
  */
void Timer_Init(void)
{
	/*开启时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);			//开启TIM2的时钟
	
	/*配置时钟源*/
	TIM_InternalClockConfig(TIM2);		//选择TIM2为内部时钟，若不调用此函数，TIM默认也为内部时钟
	
	/*时基单元初始化*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;		//时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;	//计数器模式，选择向上计数
	TIM_TimeBaseInitStructure.TIM_Period = 10000 - 1;				//计数周期，即ARR的值
	TIM_TimeBaseInitStructure.TIM_Prescaler = 7200 - 1;				//预分频器，即PSC的值
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;			//重复计数器，高级定时器才会用到
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);				//将结构体变量交给TIM_TimeBaseInit，配置TIM2的时基单元	
	
	/*中断输出配置*/
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);						//清除定时器更新标志位
																//TIM_TimeBaseInit函数末尾，手动产生了更新事件
																//若不清除此标志位，则开启中断后，会立刻进入一次中断
																//如果不介意此问题，则不清除此标志位也可
	
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);					//开启TIM2的更新中断
	
	/*NVIC中断分组*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);				//配置NVIC为分组2
																//即抢占优先级范围：0~3，响应优先级范围：0~3
																//此分组配置在整个工程中仅需调用一次
																//若有多个中断，可以把此代码放在main函数内，while循环之前
																//若调用多次配置分组的代码，则后执行的配置会覆盖先执行的配置
	
	/*NVIC配置*/
	NVIC_InitTypeDef NVIC_InitStructure;						//定义结构体变量
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;				//选择配置NVIC的TIM2线
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//指定NVIC线路使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	//指定NVIC线路的抢占优先级为2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			//指定NVIC线路的响应优先级为1
	NVIC_Init(&NVIC_InitStructure);								//将结构体变量交给NVIC_Init，配置NVIC外设
	
	/*TIM使能*/
	TIM_Cmd(TIM2, ENABLE);			//使能TIM2，定时器开始运行
}

/* 定时器中断函数，可以复制到使用它的地方
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
*/
```

**main.c — 周期读取并显示计数增量**（原课程文件：`源码/6-8 编码器接口测速/User/main.c`）

```c
#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Timer.h"
#include "Encoder.h"

int16_t Speed;			//定义速度变量

int main(void)
{
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	Timer_Init();		//定时器初始化
	Encoder_Init();		//编码器初始化
	
	/*显示静态字符串*/
	OLED_ShowString(1, 1, "Speed:");		//1行1列显示字符串Speed:
	
	while (1)
	{
		OLED_ShowSignedNum(1, 7, Speed, 5);	//不断刷新显示编码器测得的最新速度
	}
}

/**
  * 函    数：TIM2中断函数
  * 参    数：无
  * 返 回 值：无
  * 注意事项：此函数为中断函数，无需调用，中断触发后自动执行
  *           函数名为预留的指定名称，可以从启动文件复制
  *           请确保函数名正确，不能有任何差异，否则中断函数将不能进入
  */
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)		//判断是否是TIM2的更新事件触发的中断
	{
		Speed = Encoder_Get();								//每隔固定时间段读取一次编码器计数增量值，即为速度值
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);			//清除TIM2更新事件的中断标志位
															//中断标志位必须清除
															//否则中断将连续不断地触发，导致主程序卡死
	}
}
```

**工程注意：**

- `Temp = TIM_GetCounter(TIM3); TIM_SetCounter(TIM3, 0);` 是先读再清零的两步操作。如果两个操作之间来了有效编码器边沿，可能丢失增量。工程里常用**自由运行 CNT + 周期差分**替代清零计数。
- 当 16 位定时器 `ARR=65535`，且两次读取之间的真实增量绝对值小于半个模长时，可按 16 位模运算处理一次回绕并解释为带符号差值；高转速/长采样窗口下需要先检查这一条件。
- 从 `CNT` 得到的速度需要考虑编码器实际计数倍率、采样周期、减速比和滤波；控制任务每 10 ms 采样时还应关注低速量化噪声。
- 共享变量 `Speed` 在 ISR 中写、主循环中读，至少应考虑 `volatile`；正式项目还需要按数据类型宽度和任务模型设计同步。


## 12. 实际开发怎么选 TIM？常见坑汇总

### 12.1 拿到需求先判断应该用哪种模式

| 实际需求 | 建议先想到 | 重点寄存器/模块 |
|---|---|---|
| 每隔 10 ms 执行一次周期工作 | 内部时钟 + 更新中断或 TRGO | PSC、CNT、ARR、UIF、UIE、NVIC |
| 统计外部脉冲个数 | 外部时钟/外部事件计数 | ETR/TIx、滤波、PSC、CNT |
| 输出 20 kHz 电机 PWM | 输出比较 PWM | PSC、ARR、CCR、OC、CH |
| 测一个方波的频率 | 输入捕获 | CNT 时基、边沿、CCR、溢出 |
| 测 PWM 的频率和占空比 | PWM 输入模式 | 两个 CCR、复位从模式 |
| 用电机 A/B 相测速、辨向 | 编码器接口 | TI1/TI2、CNT、差分/RPM |
| 三相桥上下管互补 PWM | 高级定时器 | TIM1、互补输出、死区、Break |
| 固定相位触发 ADC | TIM 主模式 TRGO | TRGO、ADC EXTSEL/触发、DMA |

### 12.2 最常见的混淆

| 常见误解 | 准确理解 |
|---|---|
| `TIM2` 挂 APB1，所以时钟一定是 36 MHz | F103 APB1 分频不为 1 时，TIM 时钟是 PCLK1 × 2 |
| `PSC=72` 就是分频 72 | 实际分频 **PSC+1**，要分频72应写71 |
| `ARR=999` 计数999次 | 向上计数从0到999，一轮1000个节拍 |
| 一次更新事件一定进入 TIM 中断 | 是否进中断还取决于更新中断允许和 NVIC |
| `TIM_CKD_DIV1` 就是时基分频 | 时基分频主要看 PSC；CKD 用于相关滤波/死区时钟 |
| CCR 永远是 PWM 占空比 | 输出比较时是阈值；输入捕获时是边沿锁存的 CNT |
| 外部时钟与输入捕获一样 | 前者用外部边沿推动 CNT；后者通常用边沿“抓拍”内部时钟计数值 |
| GPIO 设置为输出就能发 PWM | 需正确的 TIM CH 引脚复用、通道输出使能；TIM1 还需注意 MOE |
| 教学 `Speed=80` 就是 80 RPM | PWM 教学参数不等于转速；编码器增量也要换算才是 RPM |
| 16位 CNT 回绕一定算错 | 正确的模差值方法在满足增量范围条件时可以处理回绕 |

### 12.3 一张表再看懂 TIM 所有常见模式

| 模式 | CNT 数什么？ | CCR 用来干什么？ | 输出或得到什么？ |
|---|---|---|---|
| 定时中断 | 内部时钟节拍 | 不一定用 | 周期事件/中断 |
| 外部时钟 | 外部有效脉冲 | 不一定用 | 外部脉冲计数 |
| 输出比较 | 时钟节拍 | 与 CNT 比较 | 比较动作 |
| PWM | 时钟节拍 | 与 CNT 比较 | 周期方波和占空比 |
| 输入捕获 | 通常是内部时钟节拍 | 锁存边沿时 CNT | 周期/频率/脉宽 |
| PWMI | 通常是内部时钟节拍 | 两路锁存周期和脉宽 | 频率、占空比 |
| 编码器 | A/B 相有效变化 | 通常不用 | 带方向计数增量 |

**复习自检：** 看一段 TIM 代码时，能依次回答 **①时钟从哪里来？②PSC/CNT/ARR 怎样计数？③事件如何产生？④CCR 在本模式下是什么角色？⑤CPU、中断、GPIO 或其他外设怎样得到最终结果？** 就基本能理顺它的工作链路。

---


### 12.4 复习时，顺着这五个问题排查任何 TIM 配置

1. **用哪个 TIM？** C8T6 是否有这个实例？对应 GPIO 是否真的映射到目标通道？
2. **CNT 靠谁计数？** 是 RCC 的 `CK_INT`、外部时钟模式 1/2，还是 A/B 编码器？
3. **CNT 数多快、多远？** PSC、ARR、计数方向、APB 定时器时钟是否算对？
4. **外部输入到底干什么？** 它是推动 CNT、触发捕获，还是通过 `TRGI` 复位/启动 CNT？
5. **结果如何被利用？** CCR 输出 PWM？CCR 锁存测量？UEV 进中断？TRGO 触发 ADC？

### 12.5 哪些东西要记，哪些要查手册？

**要真正记住：** PSC/CNT/ARR、CCR 的两个角色、内部计时与外部计数的区别、`TRGI` 进 / `TRGO` 出、`TS` 选输入 / `SMS` 选动作 / `MMS` 选输出。

**不用死背：** 每个 TIM 的 ITRx 具体对应哪一个别的定时器、所有 GPIO 重映射组合、全部寄存器位值、输入滤波器的每一种参数。这些都应该按 **具体 MCU 型号 + RM0008 + 数据手册** 查证。

**资料：** [STM32F103C8 产品页](https://www.st.com/en/microcontrollers-microprocessors/stm32f103c8.html) ｜ [RM0008 参考手册](https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) ｜ [AN4013 定时器介绍](https://www.st.com/resource/en/application_note/dm00042534-stm32-crossseries-timer-overview-stmicroelectronics.pdf)。

*课程 6-1～6-8 原 C 源码保留为可直接阅读的代码块；旧代码中的参数和注释不一定适合我的实际产品，正式工程应按使用场景检查。*
