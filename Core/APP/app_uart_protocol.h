#ifndef APP_UART_PROTOCOL_H
#define APP_UART_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "app_sensor.h"

#define APP_UART_CMD_SET_PERIOD   0x01U
#define APP_UART_CMD_QUERY_DATA   0x02U
#define APP_UART_CMD_SET_THRESH   0x03U
#define APP_UART_CMD_CAN_ENABLE   0x04U
#define APP_UART_CMD_QUERY_STATUS 0x05U
#define APP_UART_CMD_CLEAR_STATS  0x06U

typedef struct
{
    uint16_t sample_period_ms;
    uint16_t threshold;
    uint8_t can_enabled;
    uint8_t last_cmd;
    uint32_t rx_bytes;
    uint32_t cmd_count;
    uint32_t frame_errors;
    uint32_t checksum_errors;
    uint32_t rx_overflows;
} AppRuntimeConfig_t;

typedef struct
{
    uint32_t rx_count;
    uint32_t tx_count;
    uint32_t rx_queue_drops;
    uint32_t can_error_code;
    uint16_t last_rx_id;
    uint8_t last_rx_dlc;
    uint8_t last_rx_data[8];
    uint8_t has_rx_frame;
} AppUartCanDiag_t;

void AppUartProtocol_Init(void);
void AppUartProtocol_Process(void);
void AppUartProtocol_GetConfig(AppRuntimeConfig_t *config);
void AppUartProtocol_SetSample(const AppSensorSample_t *sample);
void AppUartProtocol_SetCanDiag(const AppUartCanDiag_t *diag);

#ifdef __cplusplus
}
#endif

#endif
