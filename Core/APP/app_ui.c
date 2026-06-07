#include "app_ui.h"

#include <stdio.h>
#include <string.h>

#include "lvgl.h"

#define UI_TRACE_ROWS 5U

static lv_obj_t *s_lbl_title;
static lv_obj_t *s_lbl_subtitle;
static lv_obj_t *s_lbl_node;
static lv_obj_t *s_status_dot;
static lv_obj_t *s_lbl_rx_count;
static lv_obj_t *s_lbl_tx_count;
static lv_obj_t *s_lbl_drop_count;
static lv_obj_t *s_lbl_error_code;
static lv_obj_t *s_lbl_trace[UI_TRACE_ROWS];
static lv_obj_t *s_lbl_decoder;
static lv_obj_t *s_lbl_action;
static lv_obj_t *s_lbl_diag;
static lv_obj_t *s_lbl_hint;

static char s_trace_text[UI_TRACE_ROWS][112];
static uint32_t s_seen_rx_count;
static uint8_t s_terminal_layout;

static void frame_to_text(char *buf, uint32_t buf_size, const char *tag,
                          const BSP_CAN_RxFrame_t *frame)
{
    uint32_t id = frame->header.StdId;
    uint32_t len = frame->header.DLC;

    if (len > 8U) {
        len = 8U;
    }

    snprintf(buf, buf_size,
             "%s  %03lX  %lu  %02X %02X %02X %02X %02X %02X %02X %02X",
             tag,
             (unsigned long)id,
             (unsigned long)len,
             frame->data[0], frame->data[1], frame->data[2], frame->data[3],
             frame->data[4], frame->data[5], frame->data[6], frame->data[7]);
}

static void push_trace(const char *tag, const BSP_CAN_RxFrame_t *frame)
{
    if (frame == 0) {
        return;
    }

    for (uint32_t i = UI_TRACE_ROWS - 1U; i > 0U; i--) {
        memcpy(s_trace_text[i], s_trace_text[i - 1U], sizeof(s_trace_text[i]));
    }

    frame_to_text(s_trace_text[0], sizeof(s_trace_text[0]), tag, frame);

    for (uint32_t i = 0; i < UI_TRACE_ROWS; i++) {
        if (s_lbl_trace[i] != 0) {
            lv_label_set_text(s_lbl_trace[i], s_trace_text[i]);
        }
    }
}

static void reset_trace(void)
{
    snprintf(s_trace_text[0], sizeof(s_trace_text[0]), "DIR  ID   DL  DATA");
    snprintf(s_trace_text[1], sizeof(s_trace_text[1]), "RX   ---  -   waiting...");
    for (uint32_t i = 2U; i < UI_TRACE_ROWS; i++) {
        snprintf(s_trace_text[i], sizeof(s_trace_text[i]), "--   ---  -   -- -- -- -- -- -- -- --");
    }

    for (uint32_t i = 0; i < UI_TRACE_ROWS; i++) {
        if (s_lbl_trace[i] != 0) {
            lv_label_set_text(s_lbl_trace[i], s_trace_text[i]);
        }
    }

    s_seen_rx_count = 0;
}

static void set_node_status(uint8_t peer_online, uint8_t has_rx, uint8_t bus_off)
{
    lv_color_t color = lv_color_make(150, 150, 150);

    if (s_lbl_node != 0) {
        if (bus_off != 0U) {
            lv_label_set_text(s_lbl_node, "BUS-OFF");
        } else if (peer_online != 0U) {
            lv_label_set_text(s_lbl_node, "PEER OK");
        } else if (has_rx != 0U) {
            lv_label_set_text(s_lbl_node, "RX OK");
        } else {
            lv_label_set_text(s_lbl_node, "WAIT RX");
        }
    }

    if (bus_off != 0U) {
        color = lv_color_make(214, 48, 49);
    } else if (peer_online != 0U) {
        color = lv_color_make(34, 160, 90);
    } else if (has_rx != 0U) {
        color = lv_color_make(42, 93, 165);
    }

    if (s_status_dot != 0) {
        lv_obj_set_style_bg_color(s_status_dot, color, 0);
    }
}

static void set_action_text(const char *text, lv_color_t color)
{
    lv_obj_t *label = (s_lbl_action != 0) ? s_lbl_action : s_lbl_hint;

    if (label == 0) {
        return;
    }

    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, color, 0);
}

static lv_obj_t *create_panel(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                              lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *panel = lv_obj_create(parent);

    lv_obj_set_pos(panel, x, y);
    lv_obj_set_size(panel, w, h);
    lv_obj_set_style_radius(panel, 6, 0);
    lv_obj_set_style_bg_color(panel, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_border_color(panel, lv_color_make(210, 218, 228), 0);
    lv_obj_set_style_pad_all(panel, 10, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    return panel;
}

static lv_obj_t *create_section_title(lv_obj_t *parent, const char *text)
{
    lv_obj_t *label = lv_label_create(parent);

    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, lv_color_make(90, 100, 116), 0);

    return label;
}

static lv_obj_t *create_stat_card(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                                  lv_coord_t w, const char *name)
{
    lv_obj_t *panel = create_panel(parent, x, y, w, 76);
    lv_obj_t *title = create_section_title(panel, name);
    lv_obj_t *value;

    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    value = lv_label_create(panel);
    lv_label_set_text(value, "0");
    lv_obj_set_style_text_font(value, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(value, lv_color_make(28, 38, 52), 0);
    lv_obj_align(value, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    return value;
}

static void create_compact_layout(lv_coord_t hor_res)
{
    lv_coord_t text_width = (hor_res > 20) ? (hor_res - 20) : hor_res;

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(246, 248, 251), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    s_lbl_title = lv_label_create(lv_scr_act());
    lv_label_set_text(s_lbl_title, "CANScope-F407 [READY]");
    lv_obj_set_style_text_font(s_lbl_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_lbl_title, lv_color_make(28, 38, 52), 0);
    lv_obj_align(s_lbl_title, LV_ALIGN_TOP_LEFT, 10, 10);

    s_lbl_subtitle = lv_label_create(lv_scr_act());
    lv_label_set_text(s_lbl_subtitle, "CAN terminal | 500kbps STD | UART/I2C diagnostics");
    lv_obj_set_style_text_font(s_lbl_subtitle, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_subtitle, lv_color_make(90, 100, 116), 0);
    lv_obj_align(s_lbl_subtitle, LV_ALIGN_TOP_LEFT, 10, 36);

    s_lbl_diag = lv_label_create(lv_scr_act());
    lv_label_set_text(s_lbl_diag, "RX 0 | TX 0 | Drop 0 | Err 0x00000000");
    lv_obj_set_style_text_font(s_lbl_diag, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_diag, lv_color_make(72, 82, 96), 0);
    lv_obj_set_width(s_lbl_diag, text_width);
    lv_label_set_long_mode(s_lbl_diag, LV_LABEL_LONG_WRAP);
    lv_obj_align(s_lbl_diag, LV_ALIGN_TOP_LEFT, 10, 70);

    for (uint32_t i = 0; i < UI_TRACE_ROWS; i++) {
        s_lbl_trace[i] = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(s_lbl_trace[i], &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(s_lbl_trace[i], lv_color_make(28, 38, 52), 0);
        lv_obj_set_width(s_lbl_trace[i], text_width);
        lv_label_set_long_mode(s_lbl_trace[i], LV_LABEL_LONG_WRAP);
        lv_obj_align(s_lbl_trace[i], LV_ALIGN_TOP_LEFT, 10, 120 + (lv_coord_t)(i * 34U));
    }

    s_lbl_decoder = lv_label_create(lv_scr_act());
    lv_label_set_text(s_lbl_decoder, "Decoder 0x123: --");
    lv_obj_set_style_text_font(s_lbl_decoder, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_decoder, lv_color_make(72, 82, 96), 0);
    lv_obj_set_width(s_lbl_decoder, text_width);
    lv_label_set_long_mode(s_lbl_decoder, LV_LABEL_LONG_WRAP);
    lv_obj_align(s_lbl_decoder, LV_ALIGN_BOTTOM_LEFT, 10, -40);

    s_lbl_hint = lv_label_create(lv_scr_act());
    lv_label_set_text(s_lbl_hint, "KEY0 replay  KEY1 clear  UART AA55  Cangaroo 0x456");
    lv_obj_set_style_text_font(s_lbl_hint, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_hint, lv_color_make(90, 100, 116), 0);
    lv_obj_align(s_lbl_hint, LV_ALIGN_BOTTOM_LEFT, 10, -12);
}

static void create_terminal_layout(lv_coord_t hor_res, lv_coord_t ver_res)
{
    const lv_coord_t margin = 12;
    const lv_coord_t gap = 8;
    const lv_coord_t header_h = 58;
    const lv_coord_t stats_h = 76;
    const lv_coord_t action_h = 42;
    const lv_coord_t trace_h = 370;
    const lv_coord_t bottom_h = 136;
    lv_coord_t usable_w = hor_res - (margin * 2);
    lv_coord_t card_w = (usable_w - (gap * 3)) / 4;
    lv_coord_t y = header_h + margin;
    lv_obj_t *header;
    lv_obj_t *panel;
    lv_obj_t *title;

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(246, 248, 251), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    header = lv_obj_create(lv_scr_act());
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_size(header, hor_res, header_h);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_make(24, 32, 44), 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    s_lbl_title = lv_label_create(header);
    lv_label_set_text(s_lbl_title, "CANScope-F407 [READY]");
    lv_obj_set_style_text_font(s_lbl_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_lbl_title, lv_color_white(), 0);
    lv_obj_align(s_lbl_title, LV_ALIGN_LEFT_MID, margin, -8);

    s_lbl_subtitle = lv_label_create(header);
    lv_label_set_text(s_lbl_subtitle, "Portable CAN terminal | Cangaroo test | UART/I2C diagnostics");
    lv_obj_set_style_text_font(s_lbl_subtitle, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_subtitle, lv_color_make(186, 198, 214), 0);
    lv_obj_align(s_lbl_subtitle, LV_ALIGN_LEFT_MID, margin, 14);

    s_lbl_node = lv_label_create(header);
    lv_label_set_text(s_lbl_node, "WAIT RX");
    lv_obj_set_style_text_font(s_lbl_node, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_node, lv_color_white(), 0);
    lv_obj_align(s_lbl_node, LV_ALIGN_RIGHT_MID, -margin, 0);

    s_status_dot = lv_obj_create(header);
    lv_obj_set_size(s_status_dot, 12, 12);
    lv_obj_set_style_radius(s_status_dot, 6, 0);
    lv_obj_set_style_border_width(s_status_dot, 0, 0);
    lv_obj_set_style_bg_color(s_status_dot, lv_color_make(150, 150, 150), 0);
    lv_obj_align_to(s_status_dot, s_lbl_node, LV_ALIGN_OUT_LEFT_MID, -8, 0);
    lv_obj_clear_flag(s_status_dot, LV_OBJ_FLAG_SCROLLABLE);

    s_lbl_rx_count = create_stat_card(lv_scr_act(), margin, y, card_w, "RX");
    s_lbl_tx_count = create_stat_card(lv_scr_act(), margin + card_w + gap, y, card_w, "TX");
    s_lbl_drop_count = create_stat_card(lv_scr_act(), margin + ((card_w + gap) * 2), y, card_w, "DROP");
    s_lbl_error_code = create_stat_card(lv_scr_act(), margin + ((card_w + gap) * 3), y, card_w, "ERR");
    lv_obj_set_style_text_font(s_lbl_error_code, &lv_font_montserrat_16, 0);

    y += stats_h + gap;
    panel = create_panel(lv_scr_act(), margin, y, usable_w, action_h);

    s_lbl_action = lv_label_create(panel);
    lv_label_set_text(s_lbl_action, "Auto TX 0x123 | Cangaroo send 0x456 | UART AA55 | KEY0 replay");
    lv_obj_set_style_text_font(s_lbl_action, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_action, lv_color_make(42, 93, 165), 0);
    lv_obj_set_width(s_lbl_action, usable_w - 20);
    lv_label_set_long_mode(s_lbl_action, LV_LABEL_LONG_DOT);
    lv_obj_align(s_lbl_action, LV_ALIGN_CENTER, 0, 0);

    y += action_h + gap;
    panel = create_panel(lv_scr_act(), margin, y, usable_w, trace_h);
    title = create_section_title(panel, "Live CAN trace");
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    for (uint32_t i = 0; i < UI_TRACE_ROWS; i++) {
        s_lbl_trace[i] = lv_label_create(panel);
        lv_obj_set_style_text_font(s_lbl_trace[i], &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(s_lbl_trace[i], (i == 0U) ?
                                    lv_color_make(90, 100, 116) :
                                    lv_color_make(28, 38, 52), 0);
        lv_obj_set_width(s_lbl_trace[i], usable_w - 20);
        lv_label_set_long_mode(s_lbl_trace[i], LV_LABEL_LONG_WRAP);
        lv_obj_align(s_lbl_trace[i], LV_ALIGN_TOP_LEFT, 0, 40 + (lv_coord_t)(i * 56U));
    }

    y += trace_h + gap;
    panel = create_panel(lv_scr_act(), margin, y, (usable_w - gap) / 2, bottom_h);
    title = create_section_title(panel, "Local data / decoded CAN");
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    s_lbl_decoder = lv_label_create(panel);
    lv_label_set_text(s_lbl_decoder, "Last RX: waiting\nSensor: waiting");
    lv_obj_set_style_text_font(s_lbl_decoder, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_decoder, lv_color_make(28, 38, 52), 0);
    lv_obj_set_width(s_lbl_decoder, ((usable_w - gap) / 2) - 20);
    lv_label_set_long_mode(s_lbl_decoder, LV_LABEL_LONG_WRAP);
    lv_obj_align(s_lbl_decoder, LV_ALIGN_TOP_LEFT, 0, 34);

    panel = create_panel(lv_scr_act(), margin + ((usable_w - gap) / 2) + gap, y,
                         (usable_w - gap) / 2, bottom_h);
    title = create_section_title(panel, "Diagnostics");
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    s_lbl_diag = lv_label_create(panel);
    lv_label_set_text(s_lbl_diag, "Bus OK | Pend 0 | Peer WAIT\nUART cmd 0 | I2C --\nAutoCAN ON");
    lv_obj_set_style_text_font(s_lbl_diag, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_lbl_diag, lv_color_make(28, 38, 52), 0);
    lv_obj_set_width(s_lbl_diag, ((usable_w - gap) / 2) - 20);
    lv_label_set_long_mode(s_lbl_diag, LV_LABEL_LONG_WRAP);
    lv_obj_align(s_lbl_diag, LV_ALIGN_TOP_LEFT, 0, 34);

    (void)ver_res;
}

void AppUi_Create(void)
{
    lv_coord_t hor_res = lv_disp_get_hor_res(NULL);
    lv_coord_t ver_res = lv_disp_get_ver_res(NULL);

    s_lbl_title = 0;
    s_lbl_subtitle = 0;
    s_lbl_node = 0;
    s_status_dot = 0;
    s_lbl_rx_count = 0;
    s_lbl_tx_count = 0;
    s_lbl_drop_count = 0;
    s_lbl_error_code = 0;
    s_lbl_decoder = 0;
    s_lbl_action = 0;
    s_lbl_diag = 0;
    s_lbl_hint = 0;
    for (uint32_t i = 0; i < UI_TRACE_ROWS; i++) {
        s_lbl_trace[i] = 0;
    }

    s_terminal_layout = ((hor_res >= 360) && (ver_res >= 650)) ? 1U : 0U;
    if (s_terminal_layout != 0U) {
        create_terminal_layout(hor_res, ver_res);
    } else {
        create_compact_layout(hor_res);
    }

    reset_trace();
}

void AppUi_Update(const AppStateSnapshot_t *state)
{
    const char *title_state = "[READY]";

    if (state == 0) {
        return;
    }

    if (state->bus_off) {
        title_state = "[BUS-OFF]";
    } else if (state->can_error_code != 0U) {
        title_state = "[CAN ERR]";
    } else if ((state->has_rx_frame) || (state->tx_count > 0U)) {
        title_state = "[BUS OK]";
    }

    lv_label_set_text_fmt(s_lbl_title, "CANScope-F407 %s", title_state);
    set_node_status(state->node_online ? 1U : 0U,
                    state->has_rx_frame ? 1U : 0U,
                    state->bus_off ? 1U : 0U);

    if ((state->has_rx_frame) && (state->rx_count != s_seen_rx_count)) {
        push_trace("RX ", &state->last_rx_frame);
        s_seen_rx_count = state->rx_count;
    }

    if (s_terminal_layout != 0U) {
        lv_label_set_text_fmt(s_lbl_rx_count, "%lu", (unsigned long)state->rx_count);
        lv_label_set_text_fmt(s_lbl_tx_count, "%lu", (unsigned long)state->tx_count);
        lv_label_set_text_fmt(s_lbl_drop_count, "%lu", (unsigned long)state->rx_queue_drops);
        lv_label_set_text_fmt(s_lbl_error_code, "%08lX", (unsigned long)state->can_error_code);
    } else {
        lv_label_set_text_fmt(s_lbl_diag,
                              "RX %lu | TX %lu | Drop %lu | Err 0x%08lX | Pend %lu\n"
                              "S %u %s | UART cmd %lu | Auto %s",
                              (unsigned long)state->rx_count,
                              (unsigned long)state->tx_count,
                              (unsigned long)state->rx_queue_drops,
                              (unsigned long)state->can_error_code,
                              (unsigned long)state->can_tx_pending_count,
                              state->sensor_value,
                              state->sensor_valid ? "OK" : "ERR",
                              (unsigned long)state->uart_cmd_count,
                              state->can_auto_enabled ? "ON" : "OFF");
    }

    if (state->has_vehicle_status) {
        lv_label_set_text_fmt(s_lbl_decoder,
                              "Decoded 0x123: %u km/h\nLight %u%% | Lamp %s\nSensor %s 0x%02X f%02X\nPeriod %ums | TH %u",
                              state->vehicle.speed_kmh,
                              state->vehicle.light_percent,
                              state->vehicle.lamp_on ? "ON" : "OFF",
                              state->sensor_valid ? "OK" : "ERR",
                              state->sensor_address,
                              state->sensor_flags,
                              state->sample_period_ms,
                              state->threshold);
    } else if (state->has_rx_frame) {
        lv_label_set_text_fmt(s_lbl_decoder,
                              "Last RX 0x%03lX DLC %lu\nData %02X %02X %02X %02X\nSensor %s 0x%02X f%02X\nPeriod %ums | TH %u",
                              (unsigned long)state->last_rx_frame.header.StdId,
                              (unsigned long)state->last_rx_frame.header.DLC,
                              state->last_rx_frame.data[0],
                              state->last_rx_frame.data[1],
                              state->last_rx_frame.data[2],
                              state->last_rx_frame.data[3],
                              state->sensor_valid ? "OK" : "ERR",
                              state->sensor_address,
                              state->sensor_flags,
                              state->sample_period_ms,
                              state->threshold);
    } else if (state->has_sensor_sample) {
        lv_label_set_text_fmt(s_lbl_decoder,
                              "Last RX: waiting\nSensor %s | value %u\nI2C 0x%02X flags 0x%02X\nPeriod %ums | TH %u",
                              state->sensor_valid ? "OK" : "ERR",
                              state->sensor_value,
                              state->sensor_address,
                              state->sensor_flags,
                              state->sample_period_ms,
                              state->threshold);
    }

    if (s_terminal_layout != 0U) {
        lv_label_set_text_fmt(s_lbl_diag,
                              "Bus %s | Pend %lu | Peer %s\nTxAbort %lu | Rec %lu | Drop %lu\nUART %lu err %lu/%lu | Auto %s\nI2C 0x%02X %s",
                              state->bus_off ? "OFF" : "OK",
                              (unsigned long)state->can_tx_pending_count,
                              state->node_online ? "OK" : "LOST",
                              (unsigned long)state->can_tx_abort_count,
                              (unsigned long)state->can_recover_count,
                              (unsigned long)state->rx_queue_drops,
                              (unsigned long)state->uart_cmd_count,
                              (unsigned long)state->uart_frame_errors,
                              (unsigned long)state->uart_checksum_errors,
                              state->can_auto_enabled ? "ON" : "OFF",
                              state->sensor_address,
                              state->sensor_valid ? "OK" : "ERR");
    }
}

void AppUi_ShowTxResult(uint8_t ok, const BSP_CAN_RxFrame_t *frame)
{
    if (frame != 0) {
        push_trace(ok ? "TX " : "TX!", frame);
    }

    lv_label_set_text(s_lbl_title,
                      ok ? "CANScope-F407 [TX OK]"
                         : "CANScope-F407 [TX FAIL]");

    if (ok) {
        set_action_text("TX queued | Auto TX continues | KEY1 clear trace",
                        lv_color_make(34, 140, 82));
    } else {
        set_action_text("TX failed: check CANable/ACK/termination, or wait mailbox free",
                        lv_color_make(196, 66, 66));
    }
}

void AppUi_ShowCleared(void)
{
    lv_label_set_text(s_lbl_title, "CANScope-F407 [CLEARED]");
    lv_label_set_text(s_lbl_decoder, "Last RX: waiting\nSensor: waiting");
    set_action_text("Auto TX 0x123 | Cangaroo send 0x456 | UART AA55 | KEY0 replay",
                    lv_color_make(42, 93, 165));

    if (s_terminal_layout != 0U) {
        lv_label_set_text(s_lbl_rx_count, "0");
        lv_label_set_text(s_lbl_tx_count, "0");
        lv_label_set_text(s_lbl_drop_count, "0");
        lv_label_set_text(s_lbl_error_code, "00000000");
        lv_label_set_text(s_lbl_diag, "Bus OK | Pend 0 | Peer WAIT\nUART cmd 0 | I2C --\nAutoCAN ON");
    } else {
        lv_label_set_text(s_lbl_diag, "RX 0 | TX 0 | Drop 0 | Err 0x00000000 | Pend 0");
    }

    set_node_status(0U, 0U, 0U);
    reset_trace();
}
