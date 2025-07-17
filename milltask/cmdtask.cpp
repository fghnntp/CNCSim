#include "cmdtask.h"
#include <chrono>
#include <iostream>
#include <libintl.h>
#include <rtapi_string.h>


CmdTask::CmdTask() : running(false) {

}

CmdTask::~CmdTask() {
    stopWork(); // Ensure thread is stopped on destruction
}

//extern int main_load(int argc, char *argv[]);

void CmdTask::doWork() {
    if (running) return; // Already running

    running = true;
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

void CmdTask::stopWork() {
    if (!running) return;

    running = false;
    cv.notify_all(); // Notify any waiting threads

    if (workerThread.joinable()) {
        workerThread.join();
    }
}

void CmdTask::process() {
    // Simulate work by incrementing counter
    counter++;
    std::string result = "Processed: " + std::to_string(counter);

    // Emit result through callback
    if (resultCallback) {
        resultCallback(result);
    }

}

void CmdTask::setResultCallback(std::function<void(const std::string&)> callback) {
    resultCallback = callback;
}

void CmdTask::setFinishedCallback(std::function<void()> callback) {
    finishedCallback = callback;
}

void CmdTask::loadfile(std::string filename)
{

}

void CmdTask::init()
{
}
