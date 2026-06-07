#include "app_uart_protocol.h"

#include <stdbool.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "usart.h"

#define UART_RX_RING_SIZE 256U
#define UART_MAX_RX_PAYLOAD 16U
#define UART_STATUS_PAYLOAD_SIZE 32U

typedef enum
{
    PARSE_HEADER1 = 0,
    PARSE_HEADER2,
    PARSE_CMD,
    PARSE_LEN,
    PARSE_PAYLOAD,
    PARSE_CHECKSUM,
} ParserState_t;

static volatile uint16_t s_head;
static volatile uint16_t s_tail;
static uint8_t s_ring[UART_RX_RING_SIZE];
static uint8_t s_rx_byte;

static AppRuntimeConfig_t s_config;
static AppSensorSample_t s_last_sample;
static AppUartCanDiag_t s_can_diag;

static ParserState_t s_parse_state;
static uint8_t s_cmd;
static uint8_t s_len;
static uint8_t s_index;
static uint8_t s_payload[UART_MAX_RX_PAYLOAD];

static uint16_t ring_next(uint16_t value)
{
    value++;
    if (value >= UART_RX_RING_SIZE) {
        value = 0U;
    }
    return value;
}

static void ring_push_from_isr(uint8_t byte)
{
    uint16_t next = ring_next(s_head);

    if (next == s_tail) {
        s_config.rx_overflows++;
        return;
    }

    s_ring[s_head] = byte;
    s_head = next;
    s_config.rx_bytes++;
}

static bool ring_pop(uint8_t *byte)
{
    if (byte == 0) {
        return false;
    }

    if (s_tail == s_head) {
        return false;
    }

    *byte = s_ring[s_tail];
    s_tail = ring_next(s_tail);
    return true;
}

static uint8_t checksum(uint8_t cmd, uint8_t len, const uint8_t *payload)
{
    uint8_t sum = (uint8_t)(cmd + len);

    for (uint32_t i = 0; i < len; i++) {
        sum = (uint8_t)(sum + payload[i]);
    }

    return sum;
}

static void send_response(uint8_t cmd, const uint8_t *payload, uint8_t len)
{
    uint8_t frame[5U + UART_STATUS_PAYLOAD_SIZE];

    if (len > UART_STATUS_PAYLOAD_SIZE) {
        len = UART_STATUS_PAYLOAD_SIZE;
    }

    frame[0] = 0xAAU;
    frame[1] = 0x55U;
    frame[2] = (uint8_t)(cmd | 0x80U);
    frame[3] = len;

    for (uint32_t i = 0; i < len; i++) {
        frame[4U + i] = payload[i];
    }

    frame[4U + len] = checksum(frame[2], len, payload);
    (void)HAL_UART_Transmit(&huart1, frame, (uint16_t)(5U + len), 50U);
}

static uint16_t read_u16_le(const uint8_t *buf)
{
    return (uint16_t)((uint16_t)buf[0] | ((uint16_t)buf[1] << 8));
}

static void write_u16_le(uint8_t *buf, uint16_t value)
{
    buf[0] = (uint8_t)(value & 0xFFU);
    buf[1] = (uint8_t)(value >> 8);
}

static void write_u32_le(uint8_t *buf, uint32_t value)
{
    buf[0] = (uint8_t)(value & 0xFFU);
    buf[1] = (uint8_t)((value >> 8) & 0xFFU);
    buf[2] = (uint8_t)((value >> 16) & 0xFFU);
    buf[3] = (uint8_t)((value >> 24) & 0xFFU);
}

static void send_status(uint8_t cmd, uint8_t status)
{
    uint8_t payload[UART_STATUS_PAYLOAD_SIZE];

    memset(payload, 0, sizeof(payload));
    payload[0] = status;
    write_u16_le(&payload[1], s_last_sample.value);
    payload[3] = s_last_sample.flags;
    payload[4] = s_last_sample.address;
    write_u16_le(&payload[5], s_config.sample_period_ms);
    write_u16_le(&payload[7], s_config.threshold);
    payload[9] = s_config.can_enabled;
    payload[10] = (uint8_t)s_config.cmd_count;
    payload[11] = (uint8_t)s_config.frame_errors;
    payload[12] = (uint8_t)s_config.checksum_errors;
    payload[13] = (uint8_t)s_config.rx_overflows;
    payload[14] = s_config.last_cmd;
    payload[15] = s_can_diag.has_rx_frame;
    write_u32_le(&payload[16], s_can_diag.rx_count);
    write_u32_le(&payload[20], s_can_diag.tx_count);
    write_u16_le(&payload[24], s_can_diag.last_rx_id);
    payload[26] = s_can_diag.last_rx_dlc;
    payload[27] = (uint8_t)s_can_diag.rx_queue_drops;
    payload[28] = s_can_diag.last_rx_data[0];
    payload[29] = s_can_diag.last_rx_data[1];
    payload[30] = s_can_diag.last_rx_data[2];
    payload[31] = s_can_diag.last_rx_data[3];

    send_response(cmd, payload, (uint8_t)sizeof(payload));
}

static void handle_command(uint8_t cmd, const uint8_t *payload, uint8_t len)
{
    uint8_t ok = 0U;

    s_config.cmd_count++;
    s_config.last_cmd = cmd;

    if (cmd == APP_UART_CMD_SET_PERIOD) {
        uint16_t period;

        if (len != 2U) {
            send_status(cmd, 1U);
            return;
        }

        period = read_u16_le(payload);
        if (period < 100U) {
            period = 100U;
        } else if (period > 5000U) {
            period = 5000U;
        }

        s_config.sample_period_ms = period;
        send_status(cmd, ok);
        return;
    }

    if (cmd == APP_UART_CMD_QUERY_DATA) {
        send_status(cmd, ok);
        return;
    }

    if (cmd == APP_UART_CMD_SET_THRESH) {
        if (len != 2U) {
            send_status(cmd, 1U);
            return;
        }

        s_config.threshold = read_u16_le(payload);
        send_status(cmd, ok);
        return;
    }

    if (cmd == APP_UART_CMD_CAN_ENABLE) {
        if (len != 1U) {
            send_status(cmd, 1U);
            return;
        }

        s_config.can_enabled = (payload[0] != 0U) ? 1U : 0U;
        send_status(cmd, ok);
        return;
    }

    if (cmd == APP_UART_CMD_QUERY_STATUS) {
        send_status(cmd, ok);
        return;
    }

    if (cmd == APP_UART_CMD_CLEAR_STATS) {
        s_config.frame_errors = 0U;
        s_config.checksum_errors = 0U;
        s_config.rx_overflows = 0U;
        send_status(cmd, ok);
        return;
    }

    send_status(cmd, 2U);
}

static void parse_byte(uint8_t byte)
{
    switch (s_parse_state) {
    case PARSE_HEADER1:
        if (byte == 0xAAU) {
            s_parse_state = PARSE_HEADER2;
        }
        break;

    case PARSE_HEADER2:
        if (byte == 0x55U) {
            s_parse_state = PARSE_CMD;
        } else {
            s_config.frame_errors++;
            s_parse_state = (byte == 0xAAU) ? PARSE_HEADER2 : PARSE_HEADER1;
        }
        break;

    case PARSE_CMD:
        s_cmd = byte;
        s_parse_state = PARSE_LEN;
        break;

    case PARSE_LEN:
        s_len = byte;
        s_index = 0U;
        if (s_len > UART_MAX_RX_PAYLOAD) {
            s_config.frame_errors++;
            s_parse_state = PARSE_HEADER1;
        } else if (s_len == 0U) {
            s_parse_state = PARSE_CHECKSUM;
        } else {
            s_parse_state = PARSE_PAYLOAD;
        }
        break;

    case PARSE_PAYLOAD:
        s_payload[s_index++] = byte;
        if (s_index >= s_len) {
            s_parse_state = PARSE_CHECKSUM;
        }
        break;

    case PARSE_CHECKSUM:
        if (byte == checksum(s_cmd, s_len, s_payload)) {
            handle_command(s_cmd, s_payload, s_len);
        } else {
            s_config.checksum_errors++;
        }
        s_parse_state = PARSE_HEADER1;
        break;

    default:
        s_parse_state = PARSE_HEADER1;
        break;
    }
}

void AppUartProtocol_Init(void)
{
    memset(&s_config, 0, sizeof(s_config));
    memset(&s_last_sample, 0, sizeof(s_last_sample));
    memset(&s_can_diag, 0, sizeof(s_can_diag));

    s_head = 0U;
    s_tail = 0U;
    s_parse_state = PARSE_HEADER1;

    s_config.sample_period_ms = 1000U;
    s_config.threshold = 1000U;
    s_config.can_enabled = 1U;

    (void)HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1U);
}

void AppUartProtocol_Process(void)
{
    uint8_t byte;

    while (ring_pop(&byte)) {
        parse_byte(byte);
    }
}

void AppUartProtocol_GetConfig(AppRuntimeConfig_t *config)
{
    if (config == 0) {
        return;
    }

    taskENTER_CRITICAL();
    *config = s_config;
    taskEXIT_CRITICAL();
}

void AppUartProtocol_SetSample(const AppSensorSample_t *sample)
{
    if (sample == 0) {
        return;
    }

    taskENTER_CRITICAL();
    s_last_sample = *sample;
    taskEXIT_CRITICAL();
}

void AppUartProtocol_SetCanDiag(const AppUartCanDiag_t *diag)
{
    if (diag == 0) {
        return;
    }

    taskENTER_CRITICAL();
    s_can_diag = *diag;
    taskEXIT_CRITICAL();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if ((huart != 0) && (huart->Instance == USART1)) {
        ring_push_from_isr(s_rx_byte);
        (void)HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1U);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if ((huart != 0) && (huart->Instance == USART1)) {
        (void)HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1U);
    }
}
