#include "milltask.h"
#include <chrono>
#include <libintl.h>
#include "emcglb.h"
#include <rtapi_string.h>




MillTask::MillTask(char* emcFile) : running(false) {
    // Constructor
    //interpBase_ = makeInterp(); emc_inifile
    emcFile_ = std::string(emcFile);
    if (emcFile_ != "") {// replace it when it's a good file
        strcpy(emc_inifile, emcFile_.c_str());
    }
}

MillTask::~MillTask() {
    stopWork(); // Ensure thread is stopped on destruction
}

void MillTask::doWork() {
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

void MillTask::stopWork() {
    if (!running) return;

    running = false;
    cv.notify_all(); // Notify any waiting threads

    if (workerThread.joinable()) {
        workerThread.join();
    }
}

void MillTask::process() {
    // Simulate work by incrementing counter
    counter++;
    std::string result = "Processed: " + std::to_string(counter);

    // Emit result through callback
    if (resultCallback) {
        resultCallback(result);
    }
}

void MillTask::setResultCallback(std::function<void(const std::string&)> callback) {
    resultCallback = callback;
}

void MillTask::setFinishedCallback(std::function<void()> callback) {
    finishedCallback = callback;
}
