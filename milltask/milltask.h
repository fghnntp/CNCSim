#ifndef _MILL_TASK_
#define _MILL_TASK_

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <string>
#include <functional>
#include <interp_base.hh>

class MillTask {
public:
    MillTask();
    ~MillTask();

    // Start the worker thread
    void doWork();

    // Stop the worker thread gracefully
    void stopWork();

    // Set callback for result ready
    void setResultCallback(std::function<void(const std::string&)> callback);

    // Set callback for finished signal
    void setFinishedCallback(std::function<void()> callback);

private:
    void process();  // Main processing function

    std::thread workerThread;
    std::atomic<bool> running{false};
    std::mutex mutex;
    std::condition_variable cv;

    // Callbacks to replace Qt signals
    std::function<void(const std::string&)> resultCallback;
    std::function<void()> finishedCallback;

    int counter = 0;

    InterpBase *interpBase_;

};

#endif // _MILL_TASK_
