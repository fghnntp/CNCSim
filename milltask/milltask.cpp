#include "milltask.h"
#include <chrono>
#include <iostream>
#include <libintl.h>
#include "emcglb.h"
#include <rtapi_string.h>
#include "emcLog.h"


MillTask::MillTask(const char* emcFile) : running(false) {
    emcFile_ = std::string(emcFile);
    init();
    EMCLog::SetLog("MillTask Init Finshed");
}

MillTask::~MillTask() {
    stopWork(); // Ensure thread is stopped on destruction
}

//extern int main_load(int argc, char *argv[]);

void MillTask::doWork() {
    if (running) return; // Already running

    running = true;
    EMCLog::SetLog("MillTask start work");
    workerThread = std::thread([this]() {

        while (running) {
            // Process work
            process();

            // Small delay to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Notify that we're finished
        if (finishedCallback) {
            finishedCallback();
        }
    });
}

void MillTask::stopWork() {
    if (!running) return;

    running = false;
    cv.notify_all(); // Notify any waiting threads

    if (workerThread.joinable()) {
        workerThread.join();
    }
}

#include "motionTask.h"
#include "emcChannel.h"

void MillTask::process() { 
    switch (state_) {
    case kIDLE:
    case kHandle:
    case kMDI:
        break;
    case kAuto:
        if (!taskMethods->load_file(filename_, error_)) {
        }
        state_ = kIDLE;
        break;
    defualt:
        break;
    }

    execCmd();

    // Emit result through callback
    if (resultCallback) {
        std::string result;
        resultCallback(result);
    }

}

void MillTask::execCmd()
{
    int res = 0;
    EMCChannel::MILLCmd cmd;
    res = EMCChannel::getMillCmd(cmd);
    if (res)
        return;

    switch (cmd) {
    case EMCChannel::kMillNone:
        break;
    case EMCChannel::kMillAuto:
        state_ = kAuto;
        break;
    default:
        break;
    }
 }

void MillTask::setResultCallback(std::function<void(const std::string&)> callback) {
    resultCallback = callback;
}

void MillTask::setFinishedCallback(std::function<void()> callback) {
    finishedCallback = callback;
}

void MillTask::loadfile(std::string filename)
{

}

int MillTask::load_file(std::string filename, std::vector<IMillTaskInterface::ToolPath> *toolPath, std::string &err)
{
    int retval = 0;
    retval = taskMethods->load_file(filename, toolPath, err);

    if (!retval)
        filename_ = filename;

    return retval;
}

int MillTask::setSts(TaskState sts)
{
    state_ = sts;
    return 0;
}

void MillTask::init()
{
    taskMethods = new EMCTask(emcFile_);
}
