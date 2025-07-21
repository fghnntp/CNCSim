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
    //Queue control the all emcchannels
public:
    enum MILLCmd{
        kMillNone,
        kMillAuto,
        kMillCmdNum,
    };

    enum MOTChannel {
        kMillChanel,
        kCmdChannel,
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
    static struct state_tag_t localEmcTrajTag;

    static int emcTrajUpdateTag(StateTag const &tag) {
        localEmcTrajTag = tag.get_state_tag();
        return 0;
    }

    static int emcTrajSetScale(double scale, enum MOTChannel = kMillChanel);
    static int emcTrajSetRapidScale(double scale, enum MOTChannel = kMillChanel);
    static int emcTrajSetMaxVelocity(double vel, enum MOTChannel = kMillChanel);

    static int emcTrajSetSpindles(int spindles, enum MOTChannel = kMillChanel);
    static int emcTrajSetJoints(int joints, enum MOTChannel = kMillChanel);
    static int  emcTrajSetVelocity(double vel, double ini_maxvel, enum MOTChannel = kMillChanel);
    static int emcTrajSetAcceleration(double acc, enum MOTChannel = kMillChanel);
    static int emcSetupArcBlends(int arcBlendEnable,
                            int arcBlendFallbackEnable,
                            int arcBlendOptDepth,
                            int arcBlendGapCycles,
                            double arcBlendRampFreq,
                            double arcBlendTangentKinkRatio, enum MOTChannel = kMillChanel);
    static int emcSetMaxFeedOverride(double maxFeedScale, enum MOTChannel = kMillChanel);
    static int emcSetProbeErrorInhibit(int j_inhibit, int h_inhibit, enum MOTChannel = kMillChanel);
    static int emcTrajSetHome(const EmcPose& home, enum MOTChannel = kMillChanel);
    static int emcJointSetBacklash(int joint, double backlash, enum MOTChannel = kMillChanel);
    static int emcJointSetMinPositionLimit(int joint, double limit, enum MOTChannel = kMillChanel);
    static int emcJointSetMaxPositionLimit(int joint, double limit, enum MOTChannel = kMillChanel);
    static int emcJointSetFerror(int joint, double ferror, enum MOTChannel = kMillChanel);
    static int emcJointSetMinFerror(int joint, double ferror, enum MOTChannel = kMillChanel);
    static int emcJointSetHomingParams(int joint, double home, double offset, double home_final_vel,
                   double search_vel, double latch_vel,
                   int use_index, int encoder_does_not_reset,
                   int ignore_limits, int is_shared,
                   int sequence,int volatile_home, int locking_indexer,int absolute_encoder, enum MOTChannel = kMillChanel);
    static int emcJointSetMaxVelocity(int joint, double vel, enum MOTChannel = kMillChanel);
    static int emcJointSetMaxAcceleration(int joint, double acc, enum MOTChannel = kMillChanel);
    static int emcJointActivate(int joint, enum MOTChannel = kMillChanel);
    static int emcAxisSetMinPositionLimit(int axis, double limit, enum MOTChannel = kMillChanel);
    static int emcAxisSetMaxPositionLimit(int axis, double limit, enum MOTChannel = kMillChanel);
    static int emcAxisSetMaxVelocity(int axis, double vel,double ext_offset_vel, enum MOTChannel = kMillChanel);
    static int emcAxisSetMaxAcceleration(int axis, double acc,double ext_offset_acc, enum MOTChannel = kMillChanel);
    static int emcAxisSetLockingJoint(int axis, int joint, enum MOTChannel = kMillChanel);
    static int emcSpindleSetParams(int spindle, double max_pos, double min_pos, double max_neg,
                   double min_neg, double search_vel, double home_angle, int sequence, double increment, enum MOTChannel = kMillChanel);


    //extend base
    static int emcMotAbort(enum MOTChannel = kMillChanel);
    static int emcJogAbort(int axis, int joint, enum MOTChannel = kMillChanel);
    static int emcMotFree(enum MOTChannel = kMillChanel);
    static int emcMotCoord(enum MOTChannel = kMillChanel);
    static int emcMotTeleop(enum MOTChannel = kMillChanel);
    static int emcMotSetWorldHome(EmcPose pose, enum MOTChannel = kMillChanel);
    static int emcMotUpdateJointHomingParas(int joint, double offset, double home, int home_sequence, enum MOTChannel = kMillChanel);
    static int emcMotOverridLimit(int joint, enum MOTChannel = kMillChanel);
    static int emcMotSetJointMotorOffset(int joint, double offset, enum MOTChannel = kMillChanel);
    static int emcMotJogCont(int joint, int axis, double vel, enum MOTChannel = kMillChanel);
    static int emcMotJogInc(int joint, int axis, double vel, double offset, enum MOTChannel = kMillChanel);
    static int emcMotJogAbs(int joint, int axis, double vel, double offset, enum MOTChannel = kMillChanel);
    static int emcMotSetTermCond(int termCond, double tolerance, enum MOTChannel = kMillChanel);
    static int emcMotSetSpindleSync(int spindle, int spindlesync, int flags, enum MOTChannel = kMillChanel);
    static int emcMotSetLine(EmcPose pos, int id, int motion_type,
                             double vel, double ini_maxvel, double acc, int turn,
                             struct state_tag_t tag, enum MOTChannel = kMillChanel);
    static int emcMotSetCircle(EmcPose pos, int id, PM_CARTESIAN center, PM_CARTESIAN normal,
                               int turn, int motion_type, double vel, double ini_maxvel,
                               double acc, struct state_tag_t tag, enum MOTChannel = kMillChanel);
    static int emcMotPause(enum MOTChannel = kMillChanel);
    static int emcMotReverse(enum MOTChannel = kMillChanel);
    static int emcMotForward(enum MOTChannel = kMillChanel);
    static int emcMotResume(enum MOTChannel = kMillChanel);
    static int emcMotStep(enum MOTChannel = kMillChanel);
    static int emcMotFSEna(enum MOTChannel = kMillChanel);
    static int emcMotFHEna(enum MOTChannel = kMillChanel);
    static int emcSpindleScale(double scale, enum MOTChannel = kMillChanel);
    static int emcMotSSEna(int mode, enum MOTChannel = kMillChanel);
    static int emcMotAFEna(int flags, enum MOTChannel = kMillChanel);
    static int emcMotDisable(enum MOTChannel = kMillChanel);
    static int emcMotEna(enum MOTChannel = kMillChanel);
    static int emcMotJointDeActive(int joint, enum MOTChannel = kMillChanel);
    static int emcMotJointHome(int joint, enum MOTChannel = kMillChanel);
    static int emcMotJointUnhome(int joint, enum MOTChannel = kMillChanel);
    static int emcMotClearProbe(enum MOTChannel = kMillChanel);
    static int emcMotProbe(enum MOTChannel = kMillChanel);
    static int emcMotRigidTap();
    static int emcMotSetDebug();
    static int emcMotSetAout();
    static int emcMotSetDout();
    static int emcMotSpindleOn();
    static int emcMotSpindleOff();
    static int emcMotSpindleOrient();
    static int emcMotSpindleIncrese();
    static int emcMotSpindleDecrese();
    static int emcMotSpindleBrakeEnage();
    static int emcMotSpindleBrakeRelease();
    static int emcMotSetJointComp();


    //

    static int getMotCmdFromMill(emcmot_command_t &cmd);

    //Thse used to control milltask
    static void emitMillCmd(MILLCmd cmd);
    static int getMillCmd(MILLCmd& cmd);

    //These used to control mottask
    //Mot mod direct cmd
    //cmdtask is speical, it directly controlled by UI
    static int getMotCmdFromCmd(emcmot_command_t &cmd);

private:
    static int commandNum;
    static emcmot_command_t emcmotCommand;
    static MessageQueue<emcmot_command_t> mill2MotQueue;
    static MessageQueue<MILLCmd> millCmdQueue;
    static MessageQueue<emcmot_command_t> cmd2MotQueue;
    static std::mutex mutex_cmd;
    EMCChannel() = delete;
};

#endif

