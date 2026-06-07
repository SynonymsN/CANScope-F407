/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : FreeRTOS task creation for CANScope-F407
  ******************************************************************************
  */
/* USER CODE END Header */

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>

#include "bsp_led.h"
#include "bsp_key.h"
#include "bsp_lcd.h"
#include "lvgl.h"
#include "lv_port_disp.h"

#include "app_can_monitor.h"
#include "app_can_protocol.h"
#include "app_sensor.h"
#include "app_state.h"
#include "app_uart_protocol.h"
#include "app_ui.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
static osThreadId_t uiTaskHandle;
static osThreadId_t canRxTaskHandle;
static osThreadId_t diagTaskHandle;
static osThreadId_t uartTaskHandle;
static osThreadId_t sensorTaskHandle;

static const osThreadAttr_t uiTask_attr = {
    .name = "Task_UI",
    .stack_size = 4096,
    .priority = (osPriority_t) osPriorityNormal,
};

static const osThreadAttr_t canRxTask_attr = {
    .name = "Task_CAN_RX",
    .stack_size = 1024,
    .priority = (osPriority_t) osPriorityAboveNormal,
};

static const osThreadAttr_t diagTask_attr = {
    .name = "Task_Diag",
    .stack_size = 768,
    .priority = (osPriority_t) osPriorityBelowNormal,
};

static const osThreadAttr_t uartTask_attr = {
    .name = "Task_UART",
    .stack_size = 1024,
    .priority = (osPriority_t) osPriorityNormal,
};

static const osThreadAttr_t sensorTask_attr = {
    .name = "Task_Sensor",
    .stack_size = 1024,
    .priority = (osPriority_t) osPriorityLow,
};
/* USER CODE END Variables */

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static void Task_UI(void *argument);
static void Task_CAN_RX(void *argument);
static void Task_Diag(void *argument);
static void Task_UART(void *argument);
static void Task_Sensor(void *argument);
static void SyncUartCanDiag(void);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

void MX_FREERTOS_Init(void)
{
  /* USER CODE BEGIN Init */
  BSP_LED_Init();
  BSP_KEY_Init();
  BSP_LCD_Init();

  lv_init();
  lv_port_disp_init();

  AppState_Init();
  AppUartProtocol_Init();
  if (AppCanMonitor_Init() != 0) {
    Error_Handler();
  }
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* USER CODE END RTOS_QUEUES */

  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  canRxTaskHandle = osThreadNew(Task_CAN_RX, NULL, &canRxTask_attr);
  diagTaskHandle = osThreadNew(Task_Diag, NULL, &diagTask_attr);
  uartTaskHandle = osThreadNew(Task_UART, NULL, &uartTask_attr);
  sensorTaskHandle = osThreadNew(Task_Sensor, NULL, &sensorTask_attr);
  uiTaskHandle = osThreadNew(Task_UI, NULL, &uiTask_attr);
  (void)canRxTaskHandle;
  (void)diagTaskHandle;
  (void)uartTaskHandle;
  (void)sensorTaskHandle;
  (void)uiTaskHandle;
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* USER CODE END RTOS_EVENTS */
}

void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  (void)argument;

  for (;;)
  {
    osDelay(1000);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

static void Task_CAN_RX(void *argument)
{
    (void)argument;

    for (;;)
    {
        BSP_CAN_RxFrame_t frame;

        if (AppCanMonitor_WaitFrame(&frame, osWaitForever) == osOK) {
            AppCanVehicleStatus_t status;
            uint32_t now = osKernelGetTickCount();

            AppState_RecordRxFrame(&frame, now);

            if (AppCanProtocol_IsHeartbeat(&frame)) {
                AppState_RecordHeartbeat(now);
            }

            if (AppCanProtocol_DecodeVehicleStatus(&frame, &status)) {
                AppState_RecordVehicleStatus(&status, now);
            }

            AppState_SetRxQueueDrops(AppCanMonitor_GetDropCount());
        }
    }
}

static void Task_Diag(void *argument)
{
    (void)argument;

    for (;;)
    {
        AppCanMonitor_ServiceBus();
        AppState_SetRxQueueDrops(AppCanMonitor_GetDropCount());
        AppState_SetCanDiag(AppCanMonitor_GetBusError(),
                            AppCanMonitor_GetRecoverCount(),
                            AppCanMonitor_IsBusOff() != 0U);
        AppState_SetCanTxDiag(AppCanMonitor_GetTxAbortCount(),
                              AppCanMonitor_GetTxPendingCount());
        AppState_UpdateDiag(osKernelGetTickCount());
        osDelay(100);
    }
}

static void Task_UART(void *argument)
{
    (void)argument;

    for (;;)
    {
        AppRuntimeConfig_t config;

        SyncUartCanDiag();
        AppUartProtocol_Process();
        AppUartProtocol_GetConfig(&config);
        AppState_SetUartDiag(&config);
        osDelay(5);
    }
}

static void SyncUartCanDiag(void)
{
    AppStateSnapshot_t snapshot;
    AppUartCanDiag_t diag;
    uint8_t dlc = 0U;

    AppState_GetSnapshot(&snapshot);
    memset(&diag, 0, sizeof(diag));

    diag.rx_count = snapshot.rx_count;
    diag.tx_count = snapshot.tx_count;
    diag.rx_queue_drops = snapshot.rx_queue_drops;
    diag.can_error_code = snapshot.can_error_code;
    diag.has_rx_frame = snapshot.has_rx_frame ? 1U : 0U;

    if (snapshot.has_rx_frame) {
        diag.last_rx_id = (uint16_t)snapshot.last_rx_frame.header.StdId;
        dlc = snapshot.last_rx_frame.header.DLC;
        if (dlc > 8U) {
            dlc = 8U;
        }
        diag.last_rx_dlc = dlc;

        for (uint32_t i = 0; i < dlc; i++) {
            diag.last_rx_data[i] = snapshot.last_rx_frame.data[i];
        }
    }

    AppUartProtocol_SetCanDiag(&diag);
}

static uint8_t payload_checksum7(const uint8_t *payload)
{
    uint8_t sum = 0U;

    for (uint32_t i = 0; i < 7U; i++) {
        sum = (uint8_t)(sum + payload[i]);
    }

    return sum;
}

static void Task_Sensor(void *argument)
{
    (void)argument;

    uint32_t last_sample_tick = 0U;
    uint8_t sequence = 0U;

    osDelay(800);
    AppSensor_Init();

    for (;;)
    {
        AppRuntimeConfig_t config;
        uint32_t now = osKernelGetTickCount();

        AppUartProtocol_GetConfig(&config);
        if (config.sample_period_ms == 0U) {
            config.sample_period_ms = 1000U;
        }

        if ((uint32_t)(now - last_sample_tick) >= config.sample_period_ms) {
            AppSensorSample_t sample;
            BSP_CAN_RxFrame_t tx_frame;
            uint8_t payload[8];

            last_sample_tick = now;
            sequence++;

            AppSensor_Poll(&sample);
            AppUartProtocol_SetSample(&sample);
            AppState_RecordSensorSample(&sample, &config);

            payload[0] = (uint8_t)(sample.value & 0xFFU);
            payload[1] = (uint8_t)(sample.value >> 8);
            payload[2] = sample.flags;
            payload[3] = sample.address;
            payload[4] = (uint8_t)(config.threshold & 0xFFU);
            payload[5] = config.can_enabled;
            payload[6] = sequence;
            payload[7] = payload_checksum7(payload);

            if (config.can_enabled != 0U) {
                if (AppCanMonitor_SendFrame(APP_CAN_ID_VEHICLE_STATUS,
                                            payload,
                                            sizeof(payload),
                                            &tx_frame) == 0U) {
                    AppState_RecordTxFrame(&tx_frame, now);
                }
            }
        }

        osDelay(20);
    }
}

static void Task_UI(void *argument)
{
    (void)argument;

    static uint8_t tx_payload[8] = {0x01, 0x02, 0x03, 0x04,
                                    0x05, 0x06, 0x07, 0x08};
    uint8_t key0_prev = KEY_RELEASED;
    uint8_t key1_prev = KEY_RELEASED;
    uint32_t last_ui_update = 0;

    AppUi_Create();

    osDelay(50);
    AppCanMonitor_StartBus();

    for (;;)
    {
        uint8_t key0_now = BSP_KEY0_Read();
        uint8_t key1_now = BSP_KEY1_Read();
        uint32_t now = osKernelGetTickCount();

        if ((key0_now == KEY_PRESSED) && (key0_prev == KEY_RELEASED)) {
            BSP_CAN_RxFrame_t tx_frame;
            AppStateSnapshot_t snapshot;
            uint8_t replay_payload[8];
            uint8_t ret;

            AppState_GetSnapshot(&snapshot);

            if ((snapshot.has_rx_frame) &&
                (snapshot.last_rx_frame.header.IDE == CAN_ID_STD) &&
                (snapshot.last_rx_frame.header.RTR == CAN_RTR_DATA) &&
                (snapshot.last_rx_frame.header.DLC <= 8U)) {
                uint8_t len = snapshot.last_rx_frame.header.DLC;

                for (uint32_t i = 0; i < len; i++) {
                    replay_payload[i] = snapshot.last_rx_frame.data[i];
                }

                ret = AppCanMonitor_SendFrame(snapshot.last_rx_frame.header.StdId,
                                              replay_payload,
                                              len,
                                              &tx_frame);
            } else {
                for (uint32_t i = 0; i < sizeof(tx_payload); i++) {
                    tx_payload[i]++;
                }

                ret = AppCanMonitor_SendTestFrame(tx_payload, sizeof(tx_payload), &tx_frame);
            }

            if (ret == 0U) {
                AppState_RecordTxFrame(&tx_frame, now);
            }
            AppUi_ShowTxResult((ret == 0U), &tx_frame);
        }

        if ((key1_now == KEY_PRESSED) && (key1_prev == KEY_RELEASED)) {
            for (uint32_t i = 0; i < sizeof(tx_payload); i++) {
                tx_payload[i] = (uint8_t)(i + 1U);
            }

            AppState_Reset();
            AppUi_ShowCleared();
        }

        if ((uint32_t)(now - last_ui_update) >= 50U) {
            AppStateSnapshot_t snapshot;

            AppState_GetSnapshot(&snapshot);
            AppUi_Update(&snapshot);
            last_ui_update = now;
        }

        key0_prev = key0_now;
        key1_prev = key1_now;

        lv_timer_handler();
        osDelay(5);
    }
}

/* USER CODE END Application */
