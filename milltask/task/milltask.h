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

//This should be asis worker, and can manage the simultion task
class MillTask {
public:
    enum TaskState {
        kIDLE,
        kHandle,
        kMDI,
        kAuto,
    };
    MillTask(const char* emcFile=nullptr);
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
    int load_file(std::string filename, std::vector<IMillTaskInterface::ToolPath>* toolPath, std::string &err);

    void active_gcodes(int active_gcodes[ACTIVE_G_CODES]) {
        return taskMethods->pinterp->active_g_codes(active_gcodes);
    }

    void active_mcodes(int active_mcodes[ACTIVE_M_CODES]) {
        return taskMethods->pinterp->active_m_codes(active_mcodes);
    }

    void active_settings(double active_settings[ACTIVE_SETTINGS]) {
        return taskMethods->pinterp->active_settings(active_settings);
    }

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
    std::string filename_;
    std::string error_;

    EMCTask *taskMethods = nullptr;
    TaskState state_ = kIDLE;

    void execCmd();
};

#endif // _MILL_TASK_
