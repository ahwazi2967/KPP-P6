
#include "cyber_gear.h"
#include <string.h>
#include <stdbool.h>


static CAN_HandleTypeDef *s_hcan    = NULL;
static uint8_t            s_host_id = 0;
static CG_Motor           s_motors[CYBERGEAR_MAX_MOTORS];
static uint8_t            s_motor_count = 0;


static uint16_t float_to_uint(float val, float min, float max){
    if (val < min) val = min;
    if (val > max) val = max;
    return (uint16_t)((val - min) * 65535.0f / (max - min));
}


static float uint_to_float(uint16_t val, float min, float max){
    return ((float)val * (max - min) / 65535.0f) + min;
}


static HAL_StatusTypeDef cg_transmit(CG_CommType type, uint16_t data_area, uint8_t target_id, const uint8_t payload[8]){
    CAN_TxHeaderTypeDef header;
    uint32_t mailbox;

    header.ExtId = ((uint32_t)type << 24) | ((uint32_t)data_area << 8) | (uint32_t)target_id;
    header.StdId = 0;
    header.IDE   = CAN_ID_EXT;
    header.RTR   = CAN_RTR_DATA;
    header.DLC   = 8;
    header.TransmitGlobalTime = DISABLE;

    uint8_t data[8] = {0};
    if (payload != NULL) memcpy(data, payload, 8);

    return HAL_CAN_AddTxMessage(s_hcan, &header, data, &mailbox);
}


HAL_StatusTypeDef cybergear_init(CAN_HandleTypeDef *hcan, uint8_t host_id){
    s_hcan    = hcan;
    s_host_id = host_id;
    s_motor_count = 0;
    memset(s_motors, 0, sizeof(s_motors));


    CAN_FilterTypeDef filter;
    filter.FilterBank           = 0;
    filter.FilterMode           = CAN_FILTERMODE_IDMASK;
    filter.FilterScale          = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh         = 0x0000;
    filter.FilterIdLow          = 0x0000;
    filter.FilterMaskIdHigh     = 0x0000;
    filter.FilterMaskIdLow      = 0x0000;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation     = ENABLE;
    filter.SlaveStartFilterBank = 14;

    HAL_StatusTypeDef st = HAL_CAN_ConfigFilter(s_hcan, &filter);
    if (st != HAL_OK) return st;

    st = HAL_CAN_Start(s_hcan);
    if (st != HAL_OK) return st;


    return HAL_CAN_ActivateNotification(s_hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
}

bool cybergear_add_motor(uint8_t motor_id){
    if (s_motor_count >= CYBERGEAR_MAX_MOTORS) return false;
    if (cybergear_get_motor(motor_id) != NULL) return true;

    CG_Motor *m = &s_motors[s_motor_count++];
    memset(m, 0, sizeof(*m));
    m->id = motor_id;
    return true;
}

CG_Motor *cybergear_get_motor(uint8_t motor_id){
    for (uint8_t i = 0; i < s_motor_count; i++) {
        if (s_motors[i].id == motor_id) return &s_motors[i];
    }
    return NULL;
}


HAL_StatusTypeDef cybergear_enable(uint8_t motor_id){
    return cg_transmit(CG_ENABLE, s_host_id, motor_id, NULL);
}

HAL_StatusTypeDef cybergear_disable(uint8_t motor_id){
    return cg_transmit(CG_DISABLE, s_host_id, motor_id, NULL);
}

HAL_StatusTypeDef cybergear_clear_fault(uint8_t motor_id){
    uint8_t p[8] = {0};
    p[0] = 1;
    return cg_transmit(CG_DISABLE, s_host_id, motor_id, p);
}

HAL_StatusTypeDef cybergear_set_zero(uint8_t motor_id){
    uint8_t p[8] = {0};
    p[0] = 1;
    return cg_transmit(CG_SET_ZERO, s_host_id, motor_id, p);
}

HAL_StatusTypeDef cybergear_set_can_id(uint8_t motor_id, uint8_t new_id){
    uint8_t p[8] = {0};
    p[0] = 1;
    uint16_t data_area = (uint16_t)((new_id << 8) | s_host_id);
    return cg_transmit(CG_SET_CAN_ID, data_area, motor_id, p);
}


HAL_StatusTypeDef cybergear_send_motion(uint8_t motor_id, float pos, float vel, float torque, float kp, float kd){
    uint16_t pos_raw = float_to_uint(pos,    -MIT_POSITION_MAX, MIT_POSITION_MAX);
    uint16_t vel_raw = float_to_uint(vel,    -MIT_VELOCITY_MAX, MIT_VELOCITY_MAX);
    uint16_t trq_raw = float_to_uint(torque, -MIT_TORQUE_MAX,   MIT_TORQUE_MAX);
    uint16_t kp_raw  = float_to_uint(kp,      0.0f,             MIT_KP_MAX);
    uint16_t kd_raw  = float_to_uint(kd,      0.0f,             MIT_KD_MAX);

    uint8_t payload[8];
    payload[0] = pos_raw >> 8;  payload[1] = pos_raw & 0xFF;
    payload[2] = vel_raw >> 8;  payload[3] = vel_raw & 0xFF;
    payload[4] = kp_raw  >> 8;  payload[5] = kp_raw  & 0xFF;
    payload[6] = kd_raw  >> 8;  payload[7] = kd_raw  & 0xFF;

    return cg_transmit(CG_CONTROL, trq_raw, motor_id, payload);
}

static bool handle_feedback(uint32_t id, const uint8_t *d)
{
    uint8_t host  = id & 0xFF;
    uint8_t motor = (id >> 8) & 0xFF;
    if (host != s_host_id) return false;

    CG_Motor *m = cybergear_get_motor(motor);
    if (m == NULL) return false;

    m->mode       = (id >> 22) & 0x03;
    m->fault_bits = (id >> 16) & 0x3F;

    uint16_t pos_raw = (d[0] << 8) | d[1];
    uint16_t vel_raw = (d[2] << 8) | d[3];
    uint16_t trq_raw = (d[4] << 8) | d[5];
    uint16_t tmp_raw = (d[6] << 8) | d[7];

    m->position    = uint_to_float(pos_raw, -MIT_POSITION_MAX, MIT_POSITION_MAX);
    m->velocity    = uint_to_float(vel_raw, -MIT_VELOCITY_MAX, MIT_VELOCITY_MAX);
    m->torque      = uint_to_float(trq_raw, -MIT_TORQUE_MAX,   MIT_TORQUE_MAX);
    m->temperature = tmp_raw / 10.0f;

    m->is_error   = (m->fault_bits != 0);
    m->online     = true;
    m->last_rx_ms = HAL_GetTick();
    m->is_updated = true;
    return true;
}

static bool handle_fault(uint32_t id, const uint8_t *d)
{
    CG_Motor *m = cybergear_get_motor((id >> 8) & 0xFF);
    if (m == NULL) m = cybergear_get_motor(id & 0xFF);
    if (m == NULL) return false;

    m->fault_value   = (uint32_t)d[0] | ((uint32_t)d[1] << 8)
                     | ((uint32_t)d[2] << 16) | ((uint32_t)d[3] << 24);
    m->warning_value = (uint32_t)d[4] | ((uint32_t)d[5] << 8)
                     | ((uint32_t)d[6] << 16) | ((uint32_t)d[7] << 24);

    if (m->fault_value != 0) m->is_error = true;
    return true;
}

bool cybergear_rx_handler(const CAN_RxHeaderTypeDef *h, const uint8_t *d)
{
    if (h->IDE != CAN_ID_EXT) return false;
    uint32_t id   = h->ExtId;
    uint8_t  type = (id >> 24) & 0x1F;

    switch (type) {
    case CG_FEEDBACK:       return handle_feedback(id, d);
    case CG_FAULT_FEEDBACK: return handle_fault(id, d);
    default:                return false;
    }
}

