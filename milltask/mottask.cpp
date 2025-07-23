#include "mottask.h"
#include <chrono>
#include <iostream>
#include <libintl.h>
#include "emcglb.h"
#include <rtapi_string.h>
#include "emcLog.h"
#include "usrmotintf.h"

MotTask::MotTask() : running(false) {
    emcFile_ = emc_inifile;
    init();
    EMCLog::SetLog("MotTask init finshed");
}

MotTask::~MotTask() {
    stopWork(); // Ensure thread is stopped on destruction
}

void MotTask::doWork() {
    if (running) return; // Already running

    running = true;

    workerThread = std::thread([this]() {


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
#include "dbuf.h"
#include "stashf.h"


/* copies error to s */
int MotTask::usrmotReadEmcmotError(char *e)
{
    /* check to see if ptr still around */
    if (emcmotError == 0) {
    return -1;
    }

    char data[EMCMOT_ERROR_LEN];
    struct dbuf d;
    dbuf_init(&d, (unsigned char *)data, EMCMOT_ERROR_LEN);

    /* returns 0 if something, -1 if not */
    int result = emcmotErrorGet(emcmotError, data);
    if(result < 0) return result;

    struct dbuf_iter di;
    dbuf_iter_init(&di, &d);

    result =  snprintdbuf(e, EMCMOT_ERROR_LEN, &di);
    if(result < 0) return result;
    return 0;
}


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

    if (emcmotStatus->tcqlen > 0.8 * DEFAULT_TC_QUEUE_SIZE) {
    }
    else {
        if (!EMCChannel::getMotCmdFromMill(*emcmotCommand))//Low priority
            MotionTask::CmdHandler();
    }

    execCmd();

    char errMsg[EMCMOT_ERROR_LEN];

    // read the emcmot error
    while (!usrmotReadEmcmotError(errMsg)) {
        EMCLog::SetLog(errMsg, 2);
    }

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

        if (emcmotStatus->commandStatus != EMCMOT_COMMAND_OK) {
            EMCChannel::clearMill2MotQueue();
            EMCLog::SetLog("EMC_COMMAND is not ok", 2);
            motTaskSts_ = kError;
        }
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

        if (emcmotStatus->commandStatus != EMCMOT_COMMAND_OK) {
            if (ofs_)
                ofs_.close();
            EMCChannel::clearMill2MotQueue();
            EMCLog::SetLog("EMC_COMMAND is not ok", 2);
            motTaskSts_ = kError;
        }
        break;
    case kStartGather:
        needWait_ = false;
        ss << std::fixed << std::setprecision(6);
        ss << "X " << emcmotStatus->carte_pos_cmd.tran.x << " " << emcmotStatus->carte_pos_cmd.tran.x <<
              " Y " << emcmotStatus->carte_pos_cmd.tran.y << " " << emcmotStatus->carte_pos_cmd.tran.x <<
              " Z " << emcmotStatus->carte_pos_cmd.tran.z << " " << emcmotStatus->carte_pos_cmd.tran.x <<
              " A " << emcmotStatus->carte_pos_cmd.a << " " << emcmotStatus->carte_pos_cmd.a <<
              " B " << emcmotStatus->carte_pos_cmd.b << " " << emcmotStatus->carte_pos_cmd.b <<
              " C " << emcmotStatus->carte_pos_cmd.c << " " << emcmotStatus->carte_pos_cmd.c << std::endl;

        ofs_ << ss.str();
        if (emcmotStatus->tcqlen == 0)
            motTaskSts_ = kEndGather;

        if (emcmotStatus->commandStatus != EMCMOT_COMMAND_OK) {
            if (ofs_)
                ofs_.close();
            EMCChannel::clearMill2MotQueue();
            EMCLog::SetLog("EMC_COMMAND is not ok", 2);
            motTaskSts_ = kError;
        }
        break;
    case kEndGather:
        needWait_ = true;
        ofs_.close();
        EMCLog::SetLog(EMCChannel::millMotFileName + " simu finished");
        motTaskSts_ = kIdle;

        if (emcmotStatus->commandStatus != EMCMOT_COMMAND_OK) {
            EMCChannel::clearMill2MotQueue();
            EMCLog::SetLog("EMC_COMMAND is not ok", 2);
            motTaskSts_ = kError;
        }
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
