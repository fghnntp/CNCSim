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
    static void getFeedrateSacle(double &rapid, double &feed);
    static void getMotionState(int &state);
    static void getMotionFlag(int &flag);
    static void getMotCmdFb(int &cmd, int &fb);
    static int isSoftLimit();
    static double getMotDtg();
    static void getRunInfo(double &vel, double &req_vel);
    static int isJogging();
    static void getStateTag(state_tag_t &tag);

    MotionTask() = delete;
};

#endif


