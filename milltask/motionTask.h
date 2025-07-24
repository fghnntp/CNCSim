#ifndef _MOTION_TASK_
#define _MOTION_TASK_

#include "emc.hh"
#include "rs274ngc_interp.hh"	// the interpreter
#include "emcIniFile.hh"
#include "emcglb.h"
#include "interpl.hh"
#include "motion.h"
#include "emcMsgQueue.h"
#include "mill_task_interface.h"

class MotionTask {
public:

    static void InitMotion();
    static void CmdHandler();
    static void MotionCtrl();

    static struct IMillTaskInterface::ToolPath getCarteCmdPos();

    MotionTask() = delete;
};

#endif


