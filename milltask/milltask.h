#ifndef _MILL_TASK_
#define _MILL_TASK_

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <string>
#include <functional>
#include <interp_base.hh>
#include "emcTask.h"
#include "emcMsgManager.h"


class MillTask {
public:
    MillTask(char* emcFile=nullptr);
    ~MillTask();

    // Start the worker thread
    void doWork();

    // Stop the worker thread gracefully
    void stopWork();

    // Set callback for result ready
    void setResultCallback(std::function<void(const std::string&)> callback);

    // Set callback for finished signal
    void setFinishedCallback(std::function<void()> callback);

    //load the file and get the previe date
    void loadfile(std::string filename);

private:
    void init(); // Once prepaing main process function
    void process();  // Main processing function

    std::thread workerThread;
    std::atomic<bool> running{false};
    std::mutex mutex;
    std::condition_variable cv;

    // Callbacks to replace Qt signals
    std::function<void(const std::string&)> resultCallback;
    std::function<void()> finishedCallback;

    int counter = 0;

    std::string emcFile_;
 public:
    EMCTask *taskMethods = nullptr;



};

#endif // _MILL_TASK_
