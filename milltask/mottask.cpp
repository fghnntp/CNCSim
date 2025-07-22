#include "mottask.h"
#include <chrono>
#include <iostream>
#include <libintl.h>
#include "emcglb.h"
#include <rtapi_string.h>
#include "emcLog.h"

MotTask::MotTask() : running(false) {
    emcFile_ = emc_inifile;
    EMCLog::SetLog("MotTask init finshed");
}

MotTask::~MotTask() {
    stopWork(); // Ensure thread is stopped on destruction
}

void MotTask::doWork() {
    if (running) return; // Already running

    running = true;

    workerThread = std::thread([this]() {
        init();

        EMCLog::SetLog("MotTask start work");
        while (running) {
            // Process work
            process();

            // Small delay to prevent busy waiting
            if (needWait_)
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }


        // Notify that we're finished
        if (finishedCallback) {
            finishedCallback();
        }
    });
}

void MotTask::stopWork() {
    if (!running) return;

    running = false;
    cv.notify_all(); // Notify any waiting threads

    if (workerThread.joinable()) {
        workerThread.join();
    }
}

#include "motionTask.h"
#include "emcChannel.h"

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

#include "motionhalctrl.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

void MotTask::process() {
    // Simulate work by incrementing counter
    //    counter++;
    //    std::string result = "Processed: " + std::to_string(counter);

    MotHalCtrl::emcmot_hal_update();
    MotHalCtrl::spindle_hal_update();
    MotHalCtrl::joint_hal_update();


    while (!EMCChannel::getMotCmdFromCmd(*emcmotCommand))//High priority
        MotionTask::CmdHandler();

    MotionTask::MotionCtrl();

    //if the msg send by milltask crated, msg will be get
    if (!EMCChannel::getMotCmdFromMill(*emcmotCommand))//Low priority
        MotionTask::CmdHandler();

    execCmd();

    std::stringstream ss;

    switch (motTaskSts_) {
    case kIdle:
        needWait_ = true;
        if (start_) {
            motTaskSts_ = kStart;
            start_ = false;
        }
        break;
    case kStart:
        needWait_ = true;
        if (emcmotStatus->tcqlen > 0)
            motTaskSts_ = kOpenFile;
        break;
    case kOpenFile:
        needWait_ = true;
        ofs_.open(EMCChannel::millMotFileName);
        if (ofs_) {
            EMCLog::SetLog(EMCChannel::millMotFileName + " simu start");
            motTaskSts_ = kStartGather;
        }
        else {
            EMCLog::SetLog(EMCChannel::millMotFileName + " simu open error");
            motTaskSts_ = kError;
        }
        break;
    case kStartGather:
        needWait_ = false;
        ss << std::fixed << std::setprecision(6);
        ss << "X " << emcmotStatus->carte_pos_cmd.tran.x << " " << emcmotStatus->carte_pos_cmd.tran.x <<
              " Y " << emcmotStatus->carte_pos_cmd.tran.y << " " << emcmotStatus->carte_pos_cmd.tran.x <<
              " Z " << emcmotStatus->carte_pos_cmd.tran.z << " " << emcmotStatus->carte_pos_cmd.tran.x << std::endl;
        ofs_ << ss.str();
        if (emcmotStatus->tcqlen == 0)
            motTaskSts_ = kEndGather;
        break;
    case kEndGather:
        needWait_ = true;
        ofs_.close();
        EMCLog::SetLog(EMCChannel::millMotFileName + " simu finished");
        motTaskSts_ = kIdle;
        break;
    case kError:
    default:
        EMCLog::SetLog(EMCChannel::millMotFileName + " simu error");
        motTaskSts_ = kIdle;
        break;

    }
}

void MotTask::execCmd()
{
    int res = 0;
    EMCChannel::MotCmd cmd;
    res = EMCChannel::getMotCmd(cmd);
    if (res)
        return;

    switch (cmd) {
    case EMCChannel::kMotNone:
        break;
    case EMCChannel::kMotStart:
        start_ = true;
        break;
    case EMCChannel::kMotRest:
        start_ = false;
        motTaskSts_ = kIdle;
        break;
    default:
        break;
    }
}

void MotTask::setResultCallback(std::function<void(const std::string&)> callback) {
    resultCallback = callback;
}

void MotTask::setFinishedCallback(std::function<void()> callback) {
    finishedCallback = callback;
}

void MotTask::loadfile(std::string filename)
{

}

void MotTask::init()
{
}
