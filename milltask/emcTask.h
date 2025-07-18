#ifndef _EMC_TASK_H_
#define _EMC_TASK_H_

//#include "canon.hh"
//#include "emc.hh"
#include "linuxcnc.h"
#include "tooldata.hh"
#include "interp_base.hh"
#include "mill_task_interface.h"

#include "motion.h"
#include "motion_struct.h"      /* emcmot_struct_t */
#include <deque>

//This is a nc simulation interface
class EMCTask {
public:
    EMCTask(std::string iniFileName);
    virtual ~EMCTask();

    // Don't copy or assign
    EMCTask(const EMCTask&) = delete;
    EMCTask& operator=(const EMCTask&) = delete;


    int random_toolchanger {0};
    const char *tooltable_filename {};
    int tool_status;

    int load_file(std::string filename, std::vector<IMillTaskInterface::ToolPath>* toolPath, std::string &err);
    int load_file(std::string filename, std::string &err);
    int simulate(std::string filename, std::string &res, std::string &err);
    void init_all(void);

private:
    //This name is used to quickly init EMCTask
    //And it will pass name to emcPars for global using
    std::string emcFileName_;


    // used to milltask evaluate
    int taskPlanError = 0;
    int taskExecuteError = 0;

    InterpBase *pinterp=0;

    //G01 is too simple to generate so just do it
    // Arch Helix Generator
    std::deque<IMillTaskInterface::ToolPath> generateG02G03(EmcPose startPose, EmcPose endPos,
                                             PM_CARTESIAN center, PM_CARTESIAN normal,
                                              int type, int turn, int plane, int motion, int segement = 36);

    std::deque<IMillTaskInterface::ToolPath> generateArc(EmcPose startPose, EmcPose endPos,
                                             PM_CARTESIAN center, PM_CARTESIAN normal,int type, int plane, int motion, int segement = 360);

    std::deque<IMillTaskInterface::ToolPath> generateHelix(EmcPose startPose, EmcPose endPose,
                                             PM_CARTESIAN center, PM_CARTESIAN normal,int type, int tune, int plane, int motion, int segement = 360);

    //emcTrajcmd generator for TP
    int emcTaskIssueTrajCmd(NMLmsg * cmd);

    void emitAllCmd();

};


#endif
