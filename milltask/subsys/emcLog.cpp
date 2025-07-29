#include "emcLog.h"

MessageQueue<EMCLog::Msg>  EMCLog::logMsgQueue;


int EMCLog::GetLog(std::string &log, int &level)
{
    if (logMsgQueue.empty())
        return 1;
    auto msg = logMsgQueue.pop();
    level = msg.level;
    log = std::move(msg.log);

    return 0;
}

int EMCLog::SetLog(std::string log, int level)
{
    Msg msg;
    msg.level = level;
    msg.log = std::move(log);

    logMsgQueue.push(std::move(msg));

    return 0;
}
