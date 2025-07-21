#ifndef _CMD_TASK_
#define _CMD_TASK_

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <string>
#include <functional>
#include "emcMsgQueue.h"

//This should be asis worker, and can manage the simultion task
class CmdTask {
public:
    using CommandFunc = std::function<std::string(const std::vector<std::string>&)>;
    CmdTask();
    ~CmdTask();

    // Start the worker thread
    void doWork();

    // Stop the worker thread gracefully
    void stopWork();

    // Set callback for result ready
    void setResultCallback(std::function<void(const std::string&)> callback);

    // Set callback for finished signal
    void setFinishedCallback(std::function<void()> callback);

    //load the file and get the previe date
    void RegisterCommand(const std::string& name, CommandFunc func);
    void SetCmd(const std::string &str);

private:
    void init(); // Once prepaing main process function
    void process();  // Main processing function

    std::thread workerThread;
    std::atomic<bool> running{false};
    std::mutex mutex;
    std::mutex mutex_reg;
    std::mutex mutex_cmd;
    std::condition_variable cv;

    // Callbacks to replace Qt signals
    std::function<void(const std::string&)> resultCallback;
    std::function<void()> finishedCallback;

    std::unordered_map<std::string, CommandFunc> _commandTable;
    std::pair<std::string, std::vector<std::string>> ParseCommand(std::string s);

    MessageQueue <std::string> cmdQueue;
    MessageQueue <std::string> resQueue;

    std::string ExecuteCommand(const std::string &rawCmd);
};

#endif // _CMD_TASK_
