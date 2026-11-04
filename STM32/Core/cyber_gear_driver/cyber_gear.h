/*
 * cyber_gear.h
 *
 *  Created on: Oct 15, 2026
 *      Author: root
 */

#ifndef CYBER_GEAR_DRIVER_CYBER_GEAR_H_
#define CYBER_GEAR_DRIVER_CYBER_GEAR_H_


#include <stdint.h>
#include <stdbool.h>
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif


#define CYBERGEAR_MAX_MOTORS   8

#define MIT_POSITION_MAX   (4.0f * 3.14159265359f)  /* rad    */
#define MIT_VELOCITY_MAX   30.0f                    /* rad/s  */
#define MIT_TORQUE_MAX     12.0f                    /* Nm     */
#define MIT_KP_MAX         500.0f
#define MIT_KD_MAX         5.0f

#define CG_PARAM_RUN_MODE      0x7005u
#define CG_PARAM_SPD_REF       0x700Au
#define CG_PARAM_LOC_REF       0x7016u

typedef enum {
    CG_GET_DEVICE_ID   = 0x00,
    CG_CONTROL         = 0x01,
    CG_FEEDBACK        = 0x02,
    CG_ENABLE          = 0x03,
    CG_DISABLE         = 0x04,
    CG_SET_ZERO        = 0x06,
    CG_SET_CAN_ID      = 0x07,
    CG_PARAM_READ      = 0x11,
    CG_PARAM_WRITE     = 0x12,
    CG_FAULT_FEEDBACK  = 0x15,
} CG_CommType;


typedef struct {
    uint8_t  id;
    float    position, velocity, torque, temperature;
    uint8_t  mode;
    uint8_t  fault_bits;
    uint32_t fault_value;
    uint32_t warning_value;
    bool     is_error;
    bool     is_updated;
    bool     online;
    uint8_t  uid[8];
    uint32_t last_rx_ms;
} CG_Motor;

HAL_StatusTypeDef cybergear_init(CAN_HandleTypeDef *hcan, uint8_t host_id);
bool              cybergear_add_motor(uint8_t motor_id);
CG_Motor         *cybergear_get_motor(uint8_t motor_id);

HAL_StatusTypeDef cybergear_enable(uint8_t motor_id);
HAL_StatusTypeDef cybergear_disable(uint8_t motor_id);
HAL_StatusTypeDef cybergear_set_zero(uint8_t motor_id);
HAL_StatusTypeDef cybergear_send_motion(uint8_t motor_id, float pos, float vel, float torque, float kp, float kd);

bool cybergear_rx_handler(const CAN_RxHeaderTypeDef *rx_header, const uint8_t *rx_data);

HAL_StatusTypeDef cybergear_clear_fault(uint8_t motor_id);
HAL_StatusTypeDef cybergear_set_can_id(uint8_t motor_id, uint8_t new_id);
HAL_StatusTypeDef cybergear_set_run_mode(uint8_t motor_id, uint8_t mode);



#endif /* CYBER_GEAR_DRIVER_CYBER_GEAR_H_ */
