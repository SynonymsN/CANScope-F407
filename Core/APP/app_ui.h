#ifndef APP_UI_H
#define APP_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "app_state.h"
#include "bsp_can.h"

void AppUi_Create(void);
void AppUi_Update(const AppStateSnapshot_t *state);
void AppUi_ShowTxResult(uint8_t ok, const BSP_CAN_RxFrame_t *frame);
void AppUi_ShowCleared(void);

#ifdef __cplusplus
}
#endif

#endif
