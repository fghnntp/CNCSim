#ifndef _RT_APP_H_
#define _RT_APP_H_

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <string>
#include <functional>

class RTAPP{
public:
    RTAPP(char* emcFile=nullptr);
    ~RTAPP();

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

    std::string emcFile_;
};

#endif // _RT_APP_H_
