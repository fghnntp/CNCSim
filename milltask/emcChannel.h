#ifndef _EMC_CHANNEL_H_
#define _EMC_CHANNEL_H_

#include "emc.hh"
#include "rs274ngc_interp.hh"	// the interpreter
#include "emcIniFile.hh"
#include "emcglb.h"
#include "interpl.hh"
#include "motion.h"
#include "emcMsgQueue.h"

class EMCChannel {
public:
    static EMC_TRAJ_SET_SCALE *emcTrajSetScaleMsg;
    static EMC_TRAJ_SET_RAPID_SCALE *emcTrajSetRapidScaleMsg;
    static EMC_TRAJ_SET_MAX_VELOCITY *emcTrajSetMaxVelocityMsg;
    static EMC_TRAJ_SET_SPINDLE_SCALE *emcTrajSetSpindleScaleMsg;
    static EMC_TRAJ_SET_VELOCITY *emcTrajSetVelocityMsg;
    static EMC_TRAJ_SET_ACCELERATION *emcTrajSetAccelerationMsg;
    static EMC_TRAJ_LINEAR_MOVE *emcTrajLinearMoveMsg;
    static EMC_TRAJ_CIRCULAR_MOVE *emcTrajCircularMoveMsg;
    static EMC_TRAJ_DELAY *emcTrajDelayMsg;
    static EMC_TRAJ_SET_TERM_COND *emcTrajSetTermCondMsg;
    static EMC_TRAJ_SET_SPINDLESYNC *emcTrajSetSpindlesyncMsg;


    static int emcTrajSetScale(double scale);
    static int emcTrajSetRapidScale(double scale);
    static int emcTrajSetMaxVelocity(double vel);


private:
    static emcmot_command_t emcmotCommand;
    static int commandNum;
    static MessageQueue<emcmot_command_t> cmdMsg;
    static void push();

    EMCChannel() = delete;
};

#endif

