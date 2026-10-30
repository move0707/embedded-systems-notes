# RTC + PWR

## RTC

CubeMX：

**Timers → RTC**

通常先在 RCC 配置 LSE，再启用 RTC。

主要 HAL：

```c
HAL_RTC_Init(&hrtc);

HAL_RTC_SetTime(
    &hrtc,
    &sTime,
    RTC_FORMAT_BIN
);

HAL_RTC_SetDate(
    &hrtc,
    &sDate,
    RTC_FORMAT_BIN
);
```

读取通常按：

```c
HAL_RTC_GetTime(...);
HAL_RTC_GetDate(...);
```

备份寄存器：

```c
HAL_RTCEx_BKUPWrite(...);
HAL_RTCEx_BKUPRead(...);
```

对应标准库：

```c
BKP_WriteBackupRegister()
BKP_ReadBackupRegister()
```

## PWR

Sleep：

```c
HAL_PWR_EnterSLEEPMode(
    PWR_MAINREGULATOR_ON,
    PWR_SLEEPENTRY_WFI
);
```

Stop：

```c
HAL_PWR_EnterSTOPMode(
    PWR_LOWPOWERREGULATOR_ON,
    PWR_STOPENTRY_WFI
);
```

Standby：

```c
HAL_PWR_EnterSTANDBYMode();
```

Stop 唤醒后常要重新恢复系统时钟：

```c
SystemClock_Config();
```
