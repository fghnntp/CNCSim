#ifndef _MOTION_HAL_CTRL_
#define _MOTION_HAL_CTRL_

#include "mot_priv.h"
/* pointer to emcmot_hal_data_t struct in HAL shmem, with all HAL data */
extern emcmot_hal_data_t *emcmot_hal_data;
extern emcmot_struct_t *emcmotStruct;
/* ptrs to either buffered copies or direct memory for command and status */
extern struct emcmot_command_t *emcmotCommand;
extern struct emcmot_status_t *emcmotStatus;
extern struct emcmot_config_t *emcmotConfig;
extern struct emcmot_internal_t *emcmotInternal;
extern struct emcmot_error_t *emcmotError;	/* unused for RT_FIFO */

class MotHalCtrl {
    //ShareMemory ctrl based, for emcmot direct control
    //and simulation contrl
    //Direct control using share memory, no waste time
public:
    static void inithaldata(void) {
//        emcmot_hal_data->joint->joint_acc_cmd = 1;

    }

    static void emcmot_hal_update(void) {
        *emcmot_hal_data->jog_inhibit = 0;
    }

    static void spindle_hal_update(void) {
        /* read spindle angle (for threading, etc) */
        int joint_num, spindle_num;
        double abs_ferror, scale;
        joint_hal_t *joint_data;
        emcmot_joint_t *joint;
        unsigned char enables;

        for (spindle_num = 0; spindle_num < emcmotConfig->numSpindles; spindle_num++){
            *emcmot_hal_data->spindle[spindle_num].spindle_revs =
                    *emcmot_hal_data->spindle[spindle_num].spindle_speed_out;
            *emcmot_hal_data->spindle[spindle_num].spindle_speed_in =
                    *emcmot_hal_data->spindle[spindle_num].spindle_speed_out;
            *emcmot_hal_data->spindle[spindle_num].spindle_is_atspeed = 1;
            *emcmot_hal_data->spindle[spindle_num].spindle_amp_fault = 0;

            *emcmot_hal_data->spindle[spindle_num].spindle_inhibit = 0;
            *emcmot_hal_data->spindle[spindle_num].spindle_amp_fault = 0;
            *emcmot_hal_data->spindle[spindle_num].spindle_orient_fault = 0;
        }
    }

    static void joint_hal_update(void) {
        int joint_num;
        joint_hal_t *joint_data;
        emcmot_joint_t *joints;
        for (joint_num = 0; joint_num < emcmotConfig->numJoints ; joint_num++) {

            joint_data = &(emcmot_hal_data->joint[joint_num]);
            *joint_data->motor_pos_fb = *joint_data->motor_pos_cmd;
            *joint_data->active = 1;
            *joint_data->in_position = 1;
            *joint_data->pos_lim_sw = 0;
            *joint_data->neg_lim_sw = 0;
            *joint_data->amp_fault = 0;
            *joint_data->error = 0;
        }
    }
};

#endif
