# CANScope-F407 重构与联调说明

## 项目定位

本工程不是把 STM32F407 开发板包装成“汽车控制器”，而是一个可以落地演示的 **便携式 CAN 总线监控、记录、回放和故障注入终端**：

- F407 作为便携终端，负责 CAN1 收发、协议解析、状态缓存、RAM 报文窗口和 LVGL 终端显示。
- CANable 作为电脑侧节点，用来发送模拟车辆状态帧、心跳帧，并抓取 F407 的测试发送帧。
- FreeRTOS 用于把中断接收、协议处理、周期诊断、GUI 刷新分开，避免把所有逻辑堆在一个 while 循环里。

## 数据流

```text
CANable / 其他 CAN 节点
  -> CAN1_RX0_IRQHandler
  -> HAL_CAN_RxFifo0MsgPendingCallback
  -> BSP_CAN_RegisterRxCallback 注册的回调
  -> FreeRTOS MessageQueue
  -> Task_CAN_RX
  -> app_can_protocol 解码
  -> app_state 保存快照
  -> Task_UI
  -> app_ui 刷新 LVGL
```

## 关键模块

| 文件 | 职责 |
| --- | --- |
| `Core/BSP/bsp_can.c/h` | CAN1 底层驱动，负责过滤器、启动、发送、接收中断回调注册、错误恢复入口 |
| `Core/APP/app_can_monitor.c/h` | CAN 接收队列、测试发送帧、队列丢帧统计、Bus-Off 检测与恢复计数 |
| `Core/APP/app_can_protocol.c/h` | 定义协议 ID，并把 CAN 原始数据解码成速度、光照、车灯状态 |
| `Core/APP/app_state.c/h` | 线程安全地保存 RX/TX 计数、最后一帧、节点在线状态、CAN 错误码 |
| `Core/APP/app_ui.c/h` | LVGL CAN 监控终端界面，只在 UI 任务里调用 LVGL API |
| `Core/Src/freertos.c` | 创建 `Task_CAN_RX`、`Task_Diag`、`Task_UI` 三个业务任务 |

## FreeRTOS 任务

| 任务 | 优先级 | 周期/触发 | 作用 |
| --- | --- | --- | --- |
| `Task_CAN_RX` | AboveNormal | 队列阻塞等待 | 取 CAN 帧、解析协议、更新状态 |
| `Task_Diag` | BelowNormal | 100 ms | 更新丢帧数、节点超时、CAN 错误码、Bus-Off 恢复计数 |
| `Task_UI` | Normal | 50 ms UI 刷新，5 ms LVGL tick loop | 处理按键、回放/注入报文、刷新 LVGL |

## CAN 协议

当前协议故意保持小而清晰，便于把协议、状态机、任务调度和 UI 之间的关系讲清楚。

| ID | 方向 | DLC | 含义 |
| --- | --- | --- | --- |
| `0x100` | CANable -> F407 | >= 1 | 心跳帧，收到后节点变为 ONLINE |
| `0x123` | CANable -> F407 | >= 3 | 车辆状态帧，`Data[0]=speed km/h`，`Data[1]=light %`，`Data[2]=lamp` |
| `0x200` | F407 -> CANable | 8 | 未收到过报文时，按 KEY0 发送的默认注入帧，每次载荷自增 |

节点在线判定：1 秒内收到 `0x100` 或 `0x123`，界面显示 ONLINE；超过 1 秒未收到，显示 TIMEOUT/OFFLINE。

KEY0 行为：如果 RAM 窗口里已经有最近一帧，按 KEY0 会回放最近一帧；如果还没有收到过报文，按 KEY0 会注入默认测试帧 `0x200`。

发送策略：CANScope 作为调试终端使用 one-shot 注入策略，并监控 3 个 bxCAN TX mailbox。若总线上没有其他节点 ACK，终端会在诊断区显示 `Pend`/`TxAbort`，并自动清理 stale mailbox，避免离线测试时前三帧占满邮箱后一直无法继续操作。

## CANable 测试

总线参数：

```text
CAN1 bitrate: 500 kbit/s
STD ID only
F407 CAN1_RX: PA11
F407 CAN1_TX: PA12
```

推荐发送：

```text
ID:   0x123
DLC:  3
Data: 3C 50 01
```

预期 LVGL 显示：

```text
CANScope-F407 [ACTIVE]
Live CAN trace: RX 123 3 3C 50 01 ...
Decoded signals: speed 60 km/h, light 80 %, lamp ON
```

心跳测试：

```text
ID:   0x100
DLC:  1
Data: 01
```

按键测试：

- 按 `KEY0`：如果已收到报文，则回放最近一帧；否则发送默认注入帧 `0x200`。
- 按 `KEY1`：清空界面 RAM 日志窗口、解码状态和计数。

## RX / DROP / ERR 验证口径

- `RX` 不是靠本机按键证明的，必须由 CANable 或另一块 CAN 节点发送真实总线帧。收到帧后，`Live CAN trace` 会出现 `RX` 行，`RX` 计数增加。
- `DROP` 表示 FreeRTOS 接收队列满导致软件丢帧。正常低速测试下应该是 0；要验证它，需要用 CANable 连续高频灌帧，让 16 深度队列来不及消费。
- `ERR` 表示 CAN 控制器错误码。正常接线、正常波特率、正常 ACK 时应该是 0；要验证它，可以断开另一节点、错误波特率、断开 CANH/CANL 或缺少终端匹配，常见 ACK 错误会体现在 `ERR` 中。
- 没接 CANable 时按 KEY0 只能证明 TX 注入路径。若没有外部 ACK，终端会显示 `TX!`、`Pend` 或 `TxAbort`，这是离线总线诊断，不等价于 RX 验证。

## RTOS 设计说明

这个项目上 RTOS 的理由不是“为了用 RTOS”，而是存在三类不同实时性需求：

- CAN 中断路径要短，只搬运数据，不做 UI 和复杂业务。
- 协议解析要及时，不能被 LVGL 刷屏拖慢。
- UI 刷新是低频任务，应该消费状态快照，而不是直接操作中断里的全局变量。
- 诊断任务需要独立周期检查节点超时、队列丢帧和总线错误。

所以 FreeRTOS 的价值体现在任务边界、队列解耦、优先级设计和可观测性，而不是简单把裸机 while 循环拆成几个函数。
