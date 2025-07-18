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
    enum MILLCmd{
        kMillNone,
        kMillAuto,
        kMillCmdNum,
    };
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

    static int emcTrajSetSpindles(int spindles);
    static int emcTrajSetJoints(int joints);
    static int  emcTrajSetVelocity(double vel, double ini_maxvel);
    static int emcTrajSetAcceleration(double acc);
    static int emcSetupArcBlends(int arcBlendEnable,
                            int arcBlendFallbackEnable,
                            int arcBlendOptDepth,
                            int arcBlendGapCycles,
                            double arcBlendRampFreq,
                            double arcBlendTangentKinkRatio);
    static int emcSetMaxFeedOverride(double maxFeedScale);
    static int emcSetProbeErrorInhibit(int j_inhibit, int h_inhibit);
    static int emcTrajSetHome(const EmcPose& home);
    static int emcJointSetBacklash(int joint, double backlash);
    static int emcJointSetMinPositionLimit(int joint, double limit);
    static int emcJointSetMaxPositionLimit(int joint, double limit);
    static int emcJointSetFerror(int joint, double ferror);
    static int emcJointSetMinFerror(int joint, double ferror);
    static int emcJointSetHomingParams(int joint, double home, double offset, double home_final_vel,
                   double search_vel, double latch_vel,
                   int use_index, int encoder_does_not_reset,
                   int ignore_limits, int is_shared,
                   int sequence,int volatile_home, int locking_indexer,int absolute_encoder);
    static int emcJointSetMaxVelocity(int joint, double vel);
    static int emcJointSetMaxAcceleration(int joint, double acc);
    static int emcJointActivate(int joint);
    static int emcAxisSetMinPositionLimit(int axis, double limit);
    static int emcAxisSetMaxPositionLimit(int axis, double limit);
    static int emcAxisSetMaxVelocity(int axis, double vel,double ext_offset_vel);
    static int emcAxisSetMaxAcceleration(int axis, double acc,double ext_offset_acc);
    static int emcAxisSetLockingJoint(int axis, int joint);
    static int emcSpindleSetParams(int spindle, double max_pos, double min_pos, double max_neg,
                   double min_neg, double search_vel, double home_angle, int sequence, double increment);

    static int pop(emcmot_command_t &cmd);

    //Thse used to control milltask
    static void emitMillCmd(MILLCmd cmd);
    static int getMillCmd(MILLCmd& cmd);

    //These used to control mottask
    //Mot mod direct cmd
    //cmdtask is speical, it directly controlled by UI
    static int getMotCmd(emcmot_command_t &cmd);
    static void motCmdAbort();
    static void motCmdEnable();
    static void motCmdDisable();

private:
    static int commandNum;
    static emcmot_command_t emcmotCommand;
    static MessageQueue<emcmot_command_t> mill2MotQueue;
    static MessageQueue<MILLCmd> millCmdQueue;
    static MessageQueue<emcmot_command_t> cmd2MotQueue;
    static emcmot_command_t cmd2MotCmd;
    static std::mutex mutex_cmd;
    EMCChannel() = delete;
};

#endif

