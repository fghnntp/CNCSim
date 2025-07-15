#ifndef _MOTION_TASK_
#define _MOTION_TASK_

#include "emc.hh"
#include "rs274ngc_interp.hh"	// the interpreter
#include "emcIniFile.hh"
#include "emcglb.h"
#include "interpl.hh"
#include "motion.h"
#include "emcMsgQueue.h"

class MotionTask {
public:


    static emcmot_command_t emcmotCommandEntry;
    static emcmot_status_t emcmotStatusEntry;
    static emcmot_config_t emcmotConfigEntry;
    static emcmot_internal_t emcmotInternalEntry;
    static emcmot_error_t emcmotErrorEntry;	/* unused for RT_FIFO */

    static emcmot_command_t *emcmotCommand;
    static emcmot_status_t *emcmotStatus;
    static emcmot_config_t *emcmotConfig;
    static emcmot_internal_t *emcmotInternal;
    static emcmot_error_t *emcmotError;	/* unused for RT_FIFO */

    /* allocate array for joint data */
    static emcmot_joint_t joints[EMCMOT_MAX_JOINTS];

    static void InitMotion();
    static void CmdHandler();


    MotionTask() = delete;

};

#endif


