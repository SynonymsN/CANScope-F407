#include "app_can_protocol.h"

bool AppCanProtocol_IsHeartbeat(const BSP_CAN_RxFrame_t *frame)
{
    return (frame != 0) &&
           (frame->header.IDE == CAN_ID_STD) &&
           (frame->header.StdId == APP_CAN_ID_HEARTBEAT) &&
           (frame->header.DLC >= 1U);
}

bool AppCanProtocol_DecodeVehicleStatus(const BSP_CAN_RxFrame_t *frame,
                                        AppCanVehicleStatus_t *status)
{
    if ((frame == 0) || (status == 0)) {
        return false;
    }

    if ((frame->header.IDE != CAN_ID_STD) || (frame->header.DLC < 3U)) {
        return false;
    }

    if ((frame->header.StdId != APP_CAN_ID_VEHICLE_STATUS) &&
        (frame->header.StdId != APP_CAN_ID_TEST_TX)) {
        return false;
    }

    status->speed_kmh = frame->data[0];
    status->light_percent = frame->data[1];
    status->lamp_on = (frame->data[2] != 0U) ? 1U : 0U;
    return true;
}
