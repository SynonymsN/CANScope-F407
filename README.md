# CANScope-F407

Portable CAN bus monitoring and diagnostics terminal based on STM32F407,
FreeRTOS, LVGL and bxCAN.

This project runs on an ALIENTEK STM32F407 Explorer board. The firmware receives
and sends standard CAN frames, decodes a small demo protocol, keeps diagnostic
state in FreeRTOS tasks and displays bus activity on a local LVGL interface.

## Features

- CAN1 standard-frame RX/TX at 500 kbit/s.
- RX FIFO0 interrupt only moves frames into a FreeRTOS queue.
- `Task_CAN_RX` decodes heartbeat and business frames.
- `Task_Diag` tracks queue drops, CAN error code, bus-off recovery, TX pending
  and TX abort count.
- `Task_UI` refreshes the LVGL terminal and handles key-triggered replay/inject.
- LVGL screen shows Live CAN trace, RX/TX counters, decoded signals and bus
  diagnostics.
- TX mailbox stale protection aborts long-pending frames when the bus has no ACK.
- CANable/Cangaroo validation supported.

## Architecture

```text
CANable / other CAN node
  -> CAN1 RX FIFO0 interrupt
  -> HAL_CAN_RxFifo0MsgPendingCallback
  -> FreeRTOS queue
  -> Task_CAN_RX
  -> protocol decode
  -> shared state snapshot
  -> Task_UI / LVGL
```

Core project files:

| Path | Purpose |
| --- | --- |
| `Core/BSP/bsp_can.c` | CAN1 driver, filter, RX callback and TX protection |
| `Core/APP/app_can_monitor.c` | RX queue, TX replay/inject and diagnostics |
| `Core/APP/app_can_protocol.c` | Demo protocol IDs and signal decoding |
| `Core/APP/app_state.c` | Thread-safe display state |
| `Core/APP/app_ui.c` | LVGL CAN monitor terminal |
| `Core/Src/freertos.c` | FreeRTOS task creation |

## CAN Protocol

| ID | Direction | DLC | Meaning |
| --- | --- | --- | --- |
| `0x100` | CANable -> F407 | >= 1 | Heartbeat frame |
| `0x123` | CANable -> F407 | >= 3 | Demo status frame: speed, light, lamp |
| `0x200` | F407 -> CANable | 8 | Default injected test frame |

Recommended validation frame:

```text
ID:   0x123
DLC:  3
Data: 3C 50 01
```

Expected decoded result:

```text
speed 60 km/h, light 80 %, lamp ON
```

## Build

Requirements:

- CMake 3.22 or later
- Ninja
- `arm-none-eabi-gcc`

Build:

```powershell
cmake --preset Debug
cmake --build --preset Debug
```

Expected artifact:

```text
build/Debug/F407_1.elf
```

## CANable Smoke Test

If the CANable device is running SLCAN firmware:

```powershell
python tools/canable_slcan_smoke.py --list
python tools/canable_slcan_smoke.py --port COM5 --duration 15
```

If the CANable device is running candleLight/gs_usb firmware, send the validation
frame with the already configured CAN tool, such as Cangaroo.

## Notes

See [docs/canscope_refactor_notes.md](docs/canscope_refactor_notes.md) for task
partitioning, protocol details and validation notes.

## Third-party Components

The repository keeps STM32 HAL/CMSIS, FreeRTOS and LVGL sources needed for a
reproducible firmware build. Third-party components retain their original
licenses.
