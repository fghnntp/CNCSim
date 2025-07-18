#include "cmdtask.h"
#include <chrono>
#include <iostream>
#include <libintl.h>
#include <rtapi_string.h>
#include "emcLog.h"
#include "Version.h"
#include "emcParas.h"
#include "emcChannel.h"

CmdTask::CmdTask() : running(false) {
    init();
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

    // Emit result through callback

    while (!cmdQueue.empty()) {
        auto cmdStr = cmdQueue.pop();

        if (cmdStr != "") {
            ExecuteCommand(cmdStr);
        }
    }

    if (resultCallback) {
        std::string result;
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
    RegisterCommand("RUN", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;

        EMCChannel::emitMillCmd(EMCChannel::kMillAuto);

        return res;
        });

    RegisterCommand("STOP", [this](const std::vector<std::string>& args) -> std::string {
       std::string res;
       std::stringstream ss;

       return res;
       });

    RegisterCommand("$$", [this](const std::vector<std::string>& args) -> std::string {
       std::string res;
       std::stringstream ss;

       return res;
       });

    RegisterCommand("VER", [this](const std::vector<std::string>& args) -> std::string {
       std::string res;
       std::stringstream ss;
       ss << "CNCSIMU Version: " << CNCSIMU::version_str << std::endl;
       ss << "MILL Version: " << CNCSIMU::MILLVersion::str << std::endl;
       ss << "TOOL Version: " << CNCSIMU::TOOLVersion::str << std::endl;
       ss << "LinuxCNC Version: " << CNCSIMU::linuxcnc_version << std::endl;
       ss << "Build Time: " << CNCSIMU::build_date << " " << CNCSIMU::build_time <<  std::endl;
       ss << "Git Hash: " << CNCSIMU::git_commit_hash;

       res = ss.str();
       return res;
       });

    RegisterCommand(".", [this](const std::vector<std::string>& args) -> std::string {
       std::string res;
       std::stringstream ss;


       res = ss.str();
       return res;
       });


    RegisterCommand(".=", [this](const std::vector<std::string>& args) -> std::string {
       std::string res;
       std::stringstream ss;

       return res;
       });

    //These cmds are important, it can directly control the emcs paras
    //It means, it can change emc paras in anytime.
    RegisterCommand("TRAJ", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;
        if (args.size() == 0) {
            res = EMCParas::showTrajParas();
        }
        else if (args.size() == 2) {
            if (args[0] == "MAXACC") {
                EMCParas::emcTrajSetMaxAcceleration_(std::stod(args[1]));
            }
            else if (args[0] == "MAXVEL") {
                EMCParas::emcTrajSetMaxVelocity_(std::stod(args[1]));
            }
            else if (args[0] == "VEL") {
                EMCParas::emcTrajSetVelocity_(std::stod(args[1]), -1.0);
            }
            else if (args[0] == "ACC") {
                EMCParas::emcTrajSetAcceleration_(std::stod(args[1]));
            }
            else;
        }
        else;

        return res;
        });

    RegisterCommand("JOINT", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;
        if (args.size() == 1) {
           res = EMCParas::showJoint(std::stoi(args[0]));
        }
        else if (args.size() == 2) {

        }
        return res;
        });

    RegisterCommand("AXIS", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;
        if (args.size() == 1) {
            res = EMCParas::showAxis(std::stoi(args[0]));
        }
            return res;
        });

    RegisterCommand("SPINDLE", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;
        if (args.size() == 1) {
           res = EMCParas::showSpindle(std::stoi(args[0]));
        }
        return res;
        });

    //EMC Cmd Direct Set
    RegisterCommand("ABORT", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;
       
        return res;
        });
}

void CmdTask::RegisterCommand(const std::string &name, CommandFunc func)
{
    std::lock_guard<std::mutex> lock(mutex_reg);
    _commandTable[name] = func;
}

std::pair<std::string, std::vector<std::string>> CmdTask::ParseCommand(std::string s)
{
    // 方法1: 使用 transform 和 toupper
    std::transform(s.begin(), s.end(), s.begin(),
    [](unsigned char c){ return std::toupper(c); });

    std::istringstream iss(s);
    std::string name;

    iss >> name;

    std::vector<std::string> args;
    std::string arg;
    while (iss >> arg) {  // C++11兼容的迭代方式
        args.push_back(arg);
    }

    return std::make_pair(name, args);
}


std::string CmdTask::ExecuteCommand(const std::string &rawCmd)
{
    typedef std::unordered_map<std::string, std::function<std::string(const std::vector<std::string>&)>> CommandTable;

    // 解析命令
    std::pair<std::string, std::vector<std::string>> parsed = ParseCommand(rawCmd);
    std::string& cmd = parsed.first;
    std::vector<std::string>& args = parsed.second;
    std::string result{"Cmd Not Found!"};

    EMCLog::SetLog("Do Cmd: " + cmd);

    // 执行注册函数
    CommandTable::const_iterator it = _commandTable.find(cmd);
    if (it != _commandTable.end()) {
        result = it->second(args);  // 调用注册的函数
    }

    resQueue.push(result);
    EMCLog::SetLog("Cmd Res: " + result, 3);

    return result;
}

void CmdTask::SetCmd(const std::string &str)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    cmdQueue.push(str);
}
