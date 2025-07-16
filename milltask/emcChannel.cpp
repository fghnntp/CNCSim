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
MessageQueue<emcmot_command_t> EMCChannel::cmdMsg;

int EMCChannel::emcTrajSetScale(double scale)
{
    if (scale < 0.0) {
       scale = 0.0;
    }

    emcmotCommand.command = EMCMOT_FEED_SCALE;
    emcmotCommand.scale = scale;
    push();

    return 0;
}

int EMCChannel::emcTrajSetRapidScale(double scale)
{

    if (scale < 0.0) {
    scale = 0.0;
    }

    emcmotCommand.command = EMCMOT_RAPID_SCALE;
    emcmotCommand.scale = scale;

    push();

    return 0;
}

int EMCChannel::emcTrajSetMaxVelocity(double vel)
{
    if (vel < 0.0) {
        vel = 0.0;
    }

    EMCParas::set_traj_maxvel(vel);

    emcmotCommand.command = EMCMOT_SET_VEL_LIMIT;
    emcmotCommand.vel = vel;

    push();
    return 0;
}

void EMCChannel::push()
{
    emcmotCommand.commandNum = ++commandNum;
    cmdMsg.push(emcmotCommand);
}

int EMCChannel::pop(emcmot_command_t &cmd)
{
    if (cmdMsg.empty())
        return 1;

    cmd = cmdMsg.pop();
    return 0;
}
