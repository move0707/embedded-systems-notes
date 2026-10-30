# CAN

CubeMX：

**Connectivity → CAN**

配置：

- Prescaler；
- Time Quantum；
- BS1；
- BS2；
- SJW；
- NVIC。

HAL 初始化：

```c
HAL_CAN_Init(&hcan);
HAL_CAN_Start(&hcan);
```

发送：

```c
HAL_CAN_AddTxMessage(
    &hcan,
    &tx_header,
    tx_data,
    &mailbox
);
```

接收：

```c
HAL_CAN_GetRxMessage(
    &hcan,
    CAN_RX_FIFO0,
    &rx_header,
    rx_data
);
```

开通知：

```c
HAL_CAN_ActivateNotification(
    &hcan,
    CAN_IT_RX_FIFO0_MSG_PENDING
);
```

回调：

```c
void HAL_CAN_RxFifo0MsgPendingCallback(
    CAN_HandleTypeDef *hcan)
{
}
```

标准库：

`CAN_Transmit()` → HAL `HAL_CAN_AddTxMessage()`  
`CAN_Receive()` → HAL `HAL_CAN_GetRxMessage()`
