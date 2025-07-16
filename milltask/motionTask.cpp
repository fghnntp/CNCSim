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
