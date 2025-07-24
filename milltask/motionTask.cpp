#include "emcParas.h"
#include "emc.hh"
#include "emcTask.h"
#include "rs274ngc_interp.hh"	// the interpreter
#include "emcIniFile.hh"
#include "emcglb.h"
#include "interpl.hh"
#include "motionTask.h"
#include "homing.h"
#include "axis.h"
#include "tp.h"
#include "motion.h"
#include "mot_priv.h"


extern int rtapi_app_main_kines(void);
extern int rtapi_app_main_motion(void);
void MotionTask::InitMotion()
{
    rtapi_app_main_kines();
    rtapi_app_main_motion();
}

static long servo_period = 1000000;
extern void emcmotCommandHandler(void *arg, long servo_period);
void MotionTask::CmdHandler()
{
    emcmotCommandHandler(NULL, servo_period);
}

void MotionTask::MotionCtrl()
{
    emcmotController(NULL, servo_period);
}

//These struct is crated by motion module, and these
//point is used to drive motion module
//motion module is quickly scanned, and alawys run
extern struct emcmot_struct_t *emcmotStruct;
/* ptrs to either buffered copies or direct memory for command and status */
extern struct emcmot_command_t *emcmotCommand;
extern struct emcmot_status_t *emcmotStatus;
extern struct emcmot_config_t *emcmotConfig;
extern struct emcmot_internal_t *emcmotInternal;
extern struct emcmot_error_t *emcmotError;	/* unused for RT_FIFO */
IMillTaskInterface::ToolPath MotionTask::getCarteCmdPos()
{
    return {emcmotStatus->carte_pos_cmd.tran.x,
                emcmotStatus->carte_pos_cmd.tran.y,
                emcmotStatus->carte_pos_cmd.tran.z,
                emcmotStatus->carte_pos_cmd.a,
                emcmotStatus->carte_pos_cmd.b,
                emcmotStatus->carte_pos_cmd.c,
                emcmotStatus->carte_pos_cmd.u,
                emcmotStatus->carte_pos_cmd.v,
                emcmotStatus->carte_pos_cmd.w};
}
