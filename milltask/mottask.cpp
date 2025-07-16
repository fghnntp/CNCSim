#include "mottask.h"
#include <chrono>
#include <iostream>
#include <libintl.h>
#include "emcglb.h"
#include <rtapi_string.h>

MotTask::MotTask() : running(false) {
    emcFile_ = emc_inifile;
}

MotTask::~MotTask() {
    stopWork(); // Ensure thread is stopped on destruction
}

void MotTask::doWork() {
    if (running) return; // Already running

    running = true;

    workerThread = std::thread([this]() {
        init();
        while (running) {
            // Process work
            process();

            // Small delay to prevent busy waiting
            //std::this_thread::sleep_for(std::chrono::milliseconds(100));
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

void MotTask::process() {
    // Simulate work by incrementing counter
    counter++;
    std::string result = "Processed: " + std::to_string(counter);


    MotionTask::MotionCtrl();
    //if the msg send by milltask crated, msg will be get
    if (!EMCChannel::pop(*emcmotCommand))
        MotionTask::CmdHandler();

    // Emit result through callback
    if (resultCallback) {
        resultCallback(result);
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
