#include "rtapp.h"
#include <chrono>
#include <iostream>
#include "config.h"
#include <libintl.h>
#include "rcs_print.hh"
#include "emcglb.h"
#include "inifile.hh"
#include <rtapi_string.h>
#include "motion.h"
#include "cmd_msg.hh"
#include "emc_nml.hh"


RTAPP::RTAPP(char* emcFile) : running(false) {
    // Constructor
    //interpBase_ = makeInterp(); emc_inifile
    emcFile_ = std::string(emcFile);
    strcpy(emc_inifile, emcFile_.c_str());
}

RTAPP::~RTAPP() {
    stopWork(); // Ensure thread is stopped on destruction
}

void RTAPP::doWork() {
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

void RTAPP::stopWork() {
    if (!running) return;

    running = false;
    cv.notify_all(); // Notify any waiting threads

    if (workerThread.joinable()) {
        workerThread.join();
    }
}

void RTAPP::process() {
    // Simulate work by incrementing counter
    counter++;
    std::string result = "Processed: " + std::to_string(counter);

    // Emit result through callback
    if (resultCallback) {
        resultCallback(result);
    }
}

void RTAPP::setResultCallback(std::function<void(const std::string&)> callback) {
    resultCallback = callback;
}

void RTAPP::setFinishedCallback(std::function<void()> callback) {
    finishedCallback = callback;
}
