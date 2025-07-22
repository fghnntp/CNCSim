#ifndef _EMC_LOG_H_
#define _EMC_LOG_H_
#include <string>
#include "emcMsgQueue.h"

//Just a log encapsulator
class EMCLog {
public:
    struct Msg {
        int level;
        std::string log;
    };
    //level 0: Log
    //level 1: warning
    //level 2: error
    static int GetLog(std::string &log, int &level);
    static int SetLog(std::string log, int level = 0);
private:
    static MessageQueue<Msg> logMsgQueue;
};

#endif
