#include "emcChannel.h"
#include "emcParas.h"

// Initialize static members
EMC_TRAJ_SET_SCALE* EMCChannel::emcTrajSetScaleMsg = nullptr;
EMC_TRAJ_SET_RAPID_SCALE* EMCChannel::emcTrajSetRapidScaleMsg = nullptr;
EMC_TRAJ_SET_MAX_VELOCITY* EMCChannel::emcTrajSetMaxVelocityMsg = nullptr;
EMC_TRAJ_SET_SPINDLE_SCALE* EMCChannel::emcTrajSetSpindleScaleMsg = nullptr;
EMC_TRAJ_SET_VELOCITY* EMCChannel::emcTrajSetVelocityMsg = nullptr;
EMC_TRAJ_SET_ACCELERATION* EMCChannel::emcTrajSetAccelerationMsg = nullptr;
EMC_TRAJ_LINEAR_MOVE* EMCChannel::emcTrajLinearMoveMsg = nullptr;
EMC_TRAJ_CIRCULAR_MOVE* EMCChannel::emcTrajCircularMoveMsg = nullptr;
EMC_TRAJ_DELAY* EMCChannel::emcTrajDelayMsg = nullptr;
EMC_TRAJ_SET_TERM_COND* EMCChannel::emcTrajSetTermCondMsg = nullptr;
EMC_TRAJ_SET_SPINDLESYNC* EMCChannel::emcTrajSetSpindlesyncMsg = nullptr;

emcmot_command_t EMCChannel::emcmotCommand = {};
int EMCChannel::commandNum = 0;
MessageQueue<emcmot_command_t> EMCChannel::mill2MotQueue;
MessageQueue<EMCChannel::MILLCmd> EMCChannel::millCmdQueue;
MessageQueue<emcmot_command_t> EMCChannel::cmd2MotQueue;
emcmot_command_t EMCChannel::cmd2MotCmd = {};
std::mutex EMCChannel::mutex_cmd;

int EMCChannel::emcTrajSetScale(double scale)
{
    if (scale < 0.0) {
       scale = 0.0;
    }

    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_FEED_SCALE;
    emcmotCommand.scale = scale;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcTrajSetRapidScale(double scale)
{
    if (scale < 0.0) {
    scale = 0.0;
    }

    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_RAPID_SCALE;
    emcmotCommand.scale = scale;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcTrajSetMaxVelocity(double vel)
{
    if (vel < 0.0) {
        vel = 0.0;
    }

    EMCParas::set_traj_maxvel(vel);

    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_VEL_LIMIT;
    emcmotCommand.vel = vel;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcTrajSetSpindles(int spindles)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_NUM_SPINDLES;
    emcmotCommand.spindle = spindles;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcTrajSetJoints(int joints)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_NUM_JOINTS;
    emcmotCommand.joint = joints;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcTrajSetVelocity(double vel, double ini_maxvel)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_VEL;
    emcmotCommand.vel = vel;
    emcmotCommand.ini_maxvel = ini_maxvel;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcTrajSetAcceleration(double acc)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_ACC;
    emcmotCommand.acc = acc;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcSetupArcBlends(int arcBlendEnable, int arcBlendFallbackEnable, int arcBlendOptDepth, int arcBlendGapCycles, double arcBlendRampFreq, double arcBlendTangentKinkRatio)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    EMCChannel::emcmotCommand.command = EMCMOT_SETUP_ARC_BLENDS;
    EMCChannel::emcmotCommand.arcBlendEnable = arcBlendEnable;
    EMCChannel::emcmotCommand.arcBlendFallbackEnable = arcBlendFallbackEnable;
    EMCChannel::emcmotCommand.arcBlendOptDepth = arcBlendOptDepth;
    EMCChannel::emcmotCommand.arcBlendGapCycles = arcBlendGapCycles;
    EMCChannel::emcmotCommand.arcBlendRampFreq = arcBlendRampFreq;
    EMCChannel::emcmotCommand.arcBlendTangentKinkRatio = arcBlendTangentKinkRatio;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcSetMaxFeedOverride(double maxFeedScale)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_MAX_FEED_OVERRIDE;
    emcmotCommand.maxFeedScale = maxFeedScale;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcSetProbeErrorInhibit(int j_inhibit, int h_inhibit)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_PROBE_ERR_INHIBIT;
    emcmotCommand.probe_jog_err_inhibit = j_inhibit;
    emcmotCommand.probe_home_err_inhibit = h_inhibit;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcTrajSetHome(const EmcPose &home)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_WORLD_HOME;
    emcmotCommand.pos = home;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcJointSetBacklash(int joint, double backlash)
{
    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_JOINT_BACKLASH;
    emcmotCommand.joint = joint;
    emcmotCommand.backlash = backlash;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcJointSetMinPositionLimit(int joint, double limit)
{
    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_JOINT_POSITION_LIMITS;
    emcmotCommand.joint = joint;
    emcmotCommand.minLimit = limit;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcJointSetMaxPositionLimit(int joint, double limit)
{
    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_JOINT_POSITION_LIMITS;
    emcmotCommand.joint = joint;
    emcmotCommand.maxLimit = limit;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcJointSetFerror(int joint, double ferror)
{
    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_JOINT_MAX_FERROR;
    emcmotCommand.joint = joint;
    emcmotCommand.maxFerror = ferror;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcJointSetMinFerror(int joint, double ferror)
{
    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_JOINT_MIN_FERROR;
    emcmotCommand.joint = joint;
    emcmotCommand.minFerror = ferror;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

#include "homing.h"
int EMCChannel::emcJointSetHomingParams(int joint, double home, double offset, double home_final_vel, double search_vel, double latch_vel, int use_index, int encoder_does_not_reset, int ignore_limits, int is_shared, int sequence, int volatile_home, int locking_indexer, int absolute_encoder)
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

    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcJointSetMaxVelocity(int joint, double vel)
{
    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_JOINT_VEL_LIMIT;
    emcmotCommand.joint = joint;
    emcmotCommand.vel = vel;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcJointSetMaxAcceleration(int joint, double acc)
{
    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_JOINT_ACC_LIMIT;
    emcmotCommand.joint = joint;
    emcmotCommand.acc = acc;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcJointActivate(int joint)
{
    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_JOINT_ACTIVATE;
    emcmotCommand.joint = joint;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcAxisSetMinPositionLimit(int axis, double limit)
{
    if (axis < 0 || axis >= EMCMOT_MAX_AXIS)
            return 0;
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_AXIS_POSITION_LIMITS;
    emcmotCommand.axis = axis;
    emcmotCommand.minLimit = limit;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcAxisSetMaxPositionLimit(int axis, double limit)
{
    if (axis < 0 || axis >= EMCMOT_MAX_AXIS)
            return 0;
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_AXIS_POSITION_LIMITS;
    emcmotCommand.axis = axis;
    emcmotCommand.maxLimit = limit;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcAxisSetMaxVelocity(int axis, double vel, double ext_offset_vel)
{
    if (axis < 0 || axis >= EMCMOT_MAX_AXIS)
            return 0;
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_AXIS_VEL_LIMIT;
    emcmotCommand.axis = axis;
    emcmotCommand.vel = vel;
    emcmotCommand.ext_offset_vel = ext_offset_vel;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcAxisSetMaxAcceleration(int axis, double acc, double ext_offset_acc)
{
    if (axis < 0 || axis >= EMCMOT_MAX_AXIS)
            return 0;
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_AXIS_ACC_LIMIT;
    emcmotCommand.axis = axis;
    emcmotCommand.acc = acc;
    emcmotCommand.ext_offset_acc = ext_offset_acc;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcAxisSetLockingJoint(int axis, int joint)
{
    if (axis < 0 || axis >= EMCMOT_MAX_AXIS)
            return 0;
    std::lock_guard<std::mutex> lock(mutex_cmd);
    emcmotCommand.command = EMCMOT_SET_AXIS_LOCKING_JOINT;
    emcmotCommand.axis    = axis;
    emcmotCommand.joint   = joint;
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::emcSpindleSetParams(int spindle, double max_pos, double min_pos, double max_neg, double min_neg, double search_vel, double home_angle, int sequence, double increment)
{
    if (spindle < 0 || spindle >= EMCMOT_MAX_SPINDLES) {
    return 0;
    }
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
    emcmotCommand.commandNum++;
    mill2MotQueue.push(emcmotCommand);

    return 0;
}

int EMCChannel::pop(emcmot_command_t &cmd)
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

int EMCChannel::getMotCmd(emcmot_command_t &cmd)
{
    if (cmd2MotQueue.empty())
        return 1;

    cmd = cmd2MotQueue.pop();
    return 0;
}

void EMCChannel::motCmdAbort()
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    cmd2MotCmd.command = EMCMOT_ABORT;
    emcmotCommand.commandNum++;
    cmd2MotQueue.push(cmd2MotCmd);
}

void EMCChannel::motCmdEnable()
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    cmd2MotCmd.command = EMCMOT_ENABLE;
    emcmotCommand.commandNum++;
    cmd2MotQueue.push(cmd2MotCmd);
}

void EMCChannel::motCmdDisable()
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    cmd2MotCmd.command = EMCMOT_DISABLE;
    emcmotCommand.commandNum++;
    cmd2MotQueue.push(cmd2MotCmd);
}
