#include "emcChannel.h"
#include "emcParas.h"

// Initialize static members
EMC_TRAJ_SET_SCALE emcTrajSetScaleMsgEntry;
EMC_TRAJ_SET_SCALE* EMCChannel::emcTrajSetScaleMsg = &emcTrajSetScaleMsgEntry;
EMC_TRAJ_SET_RAPID_SCALE emcTrajSetRapidScaleMsgEntry;
EMC_TRAJ_SET_RAPID_SCALE* EMCChannel::emcTrajSetRapidScaleMsg = &emcTrajSetRapidScaleMsgEntry;
EMC_TRAJ_SET_MAX_VELOCITY emcTrajSetMaxVelocityMsgEntry;
EMC_TRAJ_SET_MAX_VELOCITY* EMCChannel::emcTrajSetMaxVelocityMsg = &emcTrajSetMaxVelocityMsgEntry;
EMC_TRAJ_SET_SPINDLE_SCALE emcTrajSetSpindleScaleMsgEntry;
EMC_TRAJ_SET_SPINDLE_SCALE* EMCChannel::emcTrajSetSpindleScaleMsg = &emcTrajSetSpindleScaleMsgEntry;
EMC_TRAJ_SET_VELOCITY emcTrajSetVelocityMsgEntry;
EMC_TRAJ_SET_VELOCITY* EMCChannel::emcTrajSetVelocityMsg = &emcTrajSetVelocityMsgEntry;
EMC_TRAJ_SET_ACCELERATION* EMCChannel::emcTrajSetAccelerationMsg = nullptr;
EMC_TRAJ_LINEAR_MOVE emcTrajLinearMoveMsgEntry;
EMC_TRAJ_LINEAR_MOVE* EMCChannel::emcTrajLinearMoveMsg = &emcTrajLinearMoveMsgEntry;
EMC_TRAJ_CIRCULAR_MOVE emcTrajCircularMoveMsgEntry;
EMC_TRAJ_CIRCULAR_MOVE* EMCChannel::emcTrajCircularMoveMsg = &emcTrajCircularMoveMsgEntry;
EMC_TRAJ_DELAY emcTrajDelayMsgEntry;
EMC_TRAJ_DELAY* EMCChannel::emcTrajDelayMsg = &emcTrajDelayMsgEntry;
EMC_TRAJ_SET_TERM_COND emcTrajSetTermCondMsgEntry;
EMC_TRAJ_SET_TERM_COND* EMCChannel::emcTrajSetTermCondMsg = &emcTrajSetTermCondMsgEntry;
EMC_TRAJ_SET_SPINDLESYNC emcTrajSetSpindlesyncMsgEntry;
EMC_TRAJ_SET_SPINDLESYNC* EMCChannel::emcTrajSetSpindlesyncMsg = &emcTrajSetSpindlesyncMsgEntry;

state_tag_t EMCChannel::localEmcTrajTag;

emcmot_command_t EMCChannel::emcmotCommand = {};
int EMCChannel::commandNum = 0;
MessageQueue<emcmot_command_t> EMCChannel::mill2MotQueue;
MessageQueue<EMCChannel::MILLCmd> EMCChannel::millCmdQueue;
MessageQueue<emcmot_command_t> EMCChannel::cmd2MotQueue;
std::mutex EMCChannel::mutex_cmd;
MessageQueue<EMCChannel::MotCmd> EMCChannel::motCmdQueue;
std::string EMCChannel::millMotFileName;

int EMCChannel::emcTrajSetScale(double scale, enum MOTChannel channel)
{
    if (scale < 0.0) {
       scale = 0.0;
    }

    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_FEED_SCALE;
    emcmotCommand.scale = scale;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcTrajSetRapidScale(double scale, enum MOTChannel channel)
{
    if (scale < 0.0) {
    scale = 0.0;
    }

    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_RAPID_SCALE;
    emcmotCommand.scale = scale;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcTrajSetMaxVelocity(double vel, enum MOTChannel channel)
{
    if (vel < 0.0) {
        vel = 0.0;
    }

    EMCParas::set_traj_maxvel(vel);

    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_VEL_LIMIT;
    emcmotCommand.vel = vel;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcTrajSetSpindles(int spindles, enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_NUM_SPINDLES;
    emcmotCommand.spindle = spindles;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcTrajSetJoints(int joints, enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_NUM_JOINTS;
    emcmotCommand.joint = joints;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcTrajSetVelocity(double vel, double ini_maxvel, enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_VEL;
    emcmotCommand.vel = vel;
    emcmotCommand.ini_maxvel = ini_maxvel;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcTrajSetAcceleration(double acc, enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_ACC;
    emcmotCommand.acc = acc;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcSetupArcBlends(int arcBlendEnable, int arcBlendFallbackEnable, int arcBlendOptDepth, int arcBlendGapCycles, double arcBlendRampFreq, double arcBlendTangentKinkRatio, enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    EMCChannel::emcmotCommand.command = EMCMOT_SETUP_ARC_BLENDS;
    EMCChannel::emcmotCommand.arcBlendEnable = arcBlendEnable;
    EMCChannel::emcmotCommand.arcBlendFallbackEnable = arcBlendFallbackEnable;
    EMCChannel::emcmotCommand.arcBlendOptDepth = arcBlendOptDepth;
    EMCChannel::emcmotCommand.arcBlendGapCycles = arcBlendGapCycles;
    EMCChannel::emcmotCommand.arcBlendRampFreq = arcBlendRampFreq;
    EMCChannel::emcmotCommand.arcBlendTangentKinkRatio = arcBlendTangentKinkRatio;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcSetMaxFeedOverride(double maxFeedScale, enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_MAX_FEED_OVERRIDE;
    emcmotCommand.maxFeedScale = maxFeedScale;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcSetProbeErrorInhibit(int j_inhibit, int h_inhibit, enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_PROBE_ERR_INHIBIT;
    emcmotCommand.probe_jog_err_inhibit = j_inhibit;
    emcmotCommand.probe_home_err_inhibit = h_inhibit;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcTrajSetHome(const EmcPose &home, enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_WORLD_HOME;
    emcmotCommand.pos = home;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcJointSetBacklash(int joint, double backlash, enum MOTChannel channel)
{
    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_JOINT_BACKLASH;
    emcmotCommand.joint = joint;
    emcmotCommand.backlash = backlash;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcJointSetMinPositionLimit(int joint, double limit, enum MOTChannel channel)
{
    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_JOINT_POSITION_LIMITS;
    emcmotCommand.joint = joint;
    emcmotCommand.minLimit = limit;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcJointSetMaxPositionLimit(int joint, double limit, enum MOTChannel channel)
{
    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_JOINT_POSITION_LIMITS;
    emcmotCommand.joint = joint;
    emcmotCommand.maxLimit = limit;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcJointSetFerror(int joint, double ferror, enum MOTChannel channel)
{
    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_JOINT_MAX_FERROR;
    emcmotCommand.joint = joint;
    emcmotCommand.maxFerror = ferror;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcJointSetMinFerror(int joint, double ferror, enum MOTChannel channel)
{
    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_JOINT_MIN_FERROR;
    emcmotCommand.joint = joint;
    emcmotCommand.minFerror = ferror;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

#include "homing.h"
int EMCChannel::emcJointSetHomingParams(int joint, double home, double offset, double home_final_vel, double search_vel, double latch_vel, int use_index, int encoder_does_not_reset, int ignore_limits, int is_shared, int sequence, int volatile_home, int locking_indexer, int absolute_encoder, enum MOTChannel channel)
{
    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_JOINT_HOMING_PARAMS;
    emcmotCommand.joint = joint;
    emcmotCommand.home = home;
    emcmotCommand.offset = offset;
    emcmotCommand.home_final_vel = home_final_vel;
    emcmotCommand.search_vel = search_vel;
    emcmotCommand.latch_vel = latch_vel;
    emcmotCommand.flags = 0;
    emcmotCommand.home_sequence = sequence;
    emcmotCommand.volatile_home = volatile_home;
    if (use_index) {
        emcmotCommand.flags |= HOME_USE_INDEX;
    }
    if (encoder_does_not_reset) {
        emcmotCommand.flags |= HOME_INDEX_NO_ENCODER_RESET;
    }
    if (ignore_limits) {
        emcmotCommand.flags |= HOME_IGNORE_LIMITS;
    }
    if (is_shared) {
        emcmotCommand.flags |= HOME_IS_SHARED;
    }
    if (locking_indexer) {
        emcmotCommand.flags |= HOME_UNLOCK_FIRST;
    }
    if (absolute_encoder) {
        switch (absolute_encoder) {
          case 0: break;
          case 1: emcmotCommand.flags |= HOME_ABSOLUTE_ENCODER;
                  emcmotCommand.flags |= HOME_NO_REHOME;
                  break;
          case 2: emcmotCommand.flags |= HOME_ABSOLUTE_ENCODER;
                  emcmotCommand.flags |= HOME_NO_REHOME;
                  emcmotCommand.flags |= HOME_NO_FINAL_MOVE;
                  break;
          default: fprintf(stderr,
                   "Unknown option for absolute_encoder <%d>",absolute_encoder);
                  break;
        }
    }

    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcJointSetMaxVelocity(int joint, double vel, enum MOTChannel channel)
{
    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_JOINT_VEL_LIMIT;
    emcmotCommand.joint = joint;
    emcmotCommand.vel = vel;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcJointSetMaxAcceleration(int joint, double acc, enum MOTChannel channel)
{
    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_JOINT_ACC_LIMIT;
    emcmotCommand.joint = joint;
    emcmotCommand.acc = acc;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcJointActivate(int joint, enum MOTChannel channel)
{
    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_JOINT_ACTIVATE;
    emcmotCommand.joint = joint;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcAxisSetMinPositionLimit(int axis, double limit, enum MOTChannel channel)
{
    if (axis < 0 || axis >= EMCMOT_MAX_AXIS)
            return 0;
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_AXIS_POSITION_LIMITS;
    emcmotCommand.axis = axis;
    emcmotCommand.minLimit = limit;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcAxisSetMaxPositionLimit(int axis, double limit, enum MOTChannel channel)
{
    if (axis < 0 || axis >= EMCMOT_MAX_AXIS)
            return 0;
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_AXIS_POSITION_LIMITS;
    emcmotCommand.axis = axis;
    emcmotCommand.maxLimit = limit;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcAxisSetMaxVelocity(int axis, double vel, double ext_offset_vel, enum MOTChannel channel)
{
    if (axis < 0 || axis >= EMCMOT_MAX_AXIS)
            return 0;
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_AXIS_VEL_LIMIT;
    emcmotCommand.axis = axis;
    emcmotCommand.vel = vel;
    emcmotCommand.ext_offset_vel = ext_offset_vel;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcAxisSetMaxAcceleration(int axis, double acc, double ext_offset_acc, enum MOTChannel channel)
{
    if (axis < 0 || axis >= EMCMOT_MAX_AXIS)
            return 0;
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_AXIS_ACC_LIMIT;
    emcmotCommand.axis = axis;
    emcmotCommand.acc = acc;
    emcmotCommand.ext_offset_acc = ext_offset_acc;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcAxisSetLockingJoint(int axis, int joint, enum MOTChannel channel)
{
    if (axis < 0 || axis >= EMCMOT_MAX_AXIS)
            return 0;
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_AXIS_LOCKING_JOINT;
    emcmotCommand.axis    = axis;
    emcmotCommand.joint   = joint;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcSpindleSetParams(int spindle, double max_pos, double min_pos, double max_neg, double min_neg, double search_vel, double home_angle, int sequence, double increment, enum MOTChannel channel)
{
    if (spindle < 0 || spindle >= EMCMOT_MAX_SPINDLES) {
    return 0;
    }
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_SPINDLE_PARAMS;
    emcmotCommand.spindle = spindle;
    emcmotCommand.maxLimit = max_pos;
    emcmotCommand.minLimit = min_neg;
    emcmotCommand.min_pos_speed = min_pos;
    emcmotCommand.max_neg_speed = max_neg;
    emcmotCommand.home = home_angle;
    emcmotCommand.search_vel = search_vel;
    emcmotCommand.home_sequence = sequence;
    emcmotCommand.offset = increment;
    emcmotCommand.commandNum = ++commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotAbort(enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_ABORT;
    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcJogAbort(int axis, int joint, enum MOTChannel channel)
{
    if (axis < 0 || axis > EMCMOT_MAX_AXIS)
        return 1;
    if (joint < 0 || joint > EMCMOT_MAX_JOINTS)
        return 2;
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.axis = axis;
    emcmotCommand.joint = joint;
    emcmotCommand.command = EMCMOT_JOG_ABORT;
    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotFree(enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_FREE;
    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotCoord(enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_COORD;
    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotTeleop(enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_TELEOP;
    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotSetWorldHome(EmcPose pose, MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_WORLD_HOME;
    emcmotCommand.pos = pose;
    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotUpdateJointHomingParas
(int joint, double offset, double home, int home_sequence, enum MOTChannel channel)
{
    if (joint < 0 || joint > EMCMOT_MAX_JOINTS)
        return 1;
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_UPDATE_JOINT_HOMING_PARAMS;
    emcmotCommand.joint = joint;
    emcmotCommand.offset = offset;
    emcmotCommand.home = home;
    emcmotCommand.home_sequence = home_sequence;
    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotOverridLimit(int joint, enum MOTChannel channel)
{
    if (joint < 0 || joint > EMCMOT_MAX_JOINTS)
        return 1;
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_OVERRIDE_LIMITS;
    emcmotCommand.joint = joint;
    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotSetJointMotorOffset(int joint, double offset, enum MOTChannel channel)
{
    if (joint < 0 || joint > EMCMOT_MAX_JOINTS)
        return 1;
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_JOINT_MOTOR_OFFSET;
    emcmotCommand.joint = joint;
    emcmotCommand.offset = offset;
    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotJogCont(int joint, int axis, double vel, enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_JOG_CONT;
    emcmotCommand.joint = joint;
    emcmotCommand.axis = axis;
    emcmotCommand.vel = vel;
    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotJogInc(int joint, int axis, double vel, double offset, enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_JOG_INCR;
    emcmotCommand.joint = joint;
    emcmotCommand.axis = axis;
    emcmotCommand.vel = vel;
    emcmotCommand.offset = offset;
    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotJogAbs(int joint, int axis, double vel, double offset, enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_JOG_ABS;
    emcmotCommand.joint = joint;
    emcmotCommand.axis = axis;
    emcmotCommand.vel = vel;
    emcmotCommand.offset = offset;
    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotSetTermCond(int termCond, double tolerance, enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_TERM_COND;
    emcmotCommand.termCond = termCond;
    emcmotCommand.tolerance = tolerance;
    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotSetSpindleSync(int spindle, int spindlesync, int flags, enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_SPINDLESYNC;
    emcmotCommand.spindle = spindle;
    emcmotCommand.spindlesync = spindlesync;
    emcmotCommand.flags = flags;
    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotSetLine
(EmcPose pos, int id, int motion_type, double vel,
 double ini_maxvel, double acc,
 int turn, state_tag_t tag, enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_LINE;
    emcmotCommand.pos = pos;
    emcmotCommand.id = id;
    emcmotCommand.motion_type = motion_type;
    emcmotCommand.vel = vel;
    emcmotCommand.ini_maxvel = ini_maxvel;
    emcmotCommand.acc = acc;
    emcmotCommand.turn = turn;
    emcmotCommand.tag = tag;

    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotSetCircle
(EmcPose pos, int id, PM_CARTESIAN center, PM_CARTESIAN normal,
int turn, int motion_type, double vel, double ini_maxvel,
 double acc, state_tag_t tag, enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_CIRCLE;
    emcmotCommand.pos = pos;
    emcmotCommand.id = id;
    emcmotCommand.center = {center.x, center.y, center.z};
    emcmotCommand.normal = {normal.x, normal.y, normal.z};
    emcmotCommand.turn = turn;
    emcmotCommand.motion_type = motion_type;
    emcmotCommand.vel = vel;
    emcmotCommand.ini_maxvel = ini_maxvel;
    emcmotCommand.acc = acc;
    emcmotCommand.tag = tag;

    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotPause(enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_PAUSE;

    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotReverse(enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_REVERSE;

    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotForward(enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_FORWARD;

    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotResume(enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_RESUME;

    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotStep(enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_STEP;

    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotFSEna(enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_FS_ENABLE;

    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotFHEna(enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_FH_ENABLE;

    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcSpindleScale(double scale, enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SPINDLE_SCALE;

    emcmotCommand.scale = scale;
    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotSSEna(int mode, enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SS_ENABLE;

    emcmotCommand.mode = mode;
    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotAFEna(int flags, enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_AF_ENABLE;

    emcmotCommand.flags = flags;
    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotDisable(enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_DISABLE;

    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotEna(enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_ENABLE;

    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotJointDeActive(int joint, enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_JOINT_DEACTIVATE;

    emcmotCommand.joint = joint;
    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotJointHome(int joint, enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_JOINT_HOME;

    emcmotCommand.joint = joint;
    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotJointUnhome(int joint, enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_JOINT_UNHOME;

    emcmotCommand.joint = joint;
    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotClearProbe(enum MOTChannel channel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_CLEAR_PROBE_FLAGS;

    emcmotCommand.commandNum = ++commandNum;
    emcmotCommand.commandNum = commandNum;
    if (channel == kMillChanel)
        mill2MotQueue.push(emcmotCommand);
    else if (channel == kCmdChannel)
        cmd2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcMotProbe(enum MOTChannel channel)
{//Todo

    return 0;
}




int EMCChannel::getMotCmdFromMill(emcmot_command_t &cmd)
{
    if (mill2MotQueue.empty())
        return 1;

    cmd = mill2MotQueue.pop();
    return 0;
}

void EMCChannel::emitMillCmd(MILLCmd cmd)
{
    millCmdQueue.push(cmd);
}

int EMCChannel::getMillCmd(MILLCmd &cmd)
{
    if (millCmdQueue.empty())
        return 1;
    cmd = millCmdQueue.pop();
    return 0;
}

void EMCChannel::emitMotCmd(MotCmd cmd)
{
    motCmdQueue.push(cmd);
}

int EMCChannel::getMotCmd(MotCmd &cmd)
{
    if (motCmdQueue.empty())
        return 1;
    cmd = motCmdQueue.pop();
    return 0;
}

int EMCChannel::getMotCmdFromCmd(emcmot_command_t &cmd)
{
    if (cmd2MotQueue.empty())
        return 1;

    cmd = cmd2MotQueue.pop();
    return 0;
}
