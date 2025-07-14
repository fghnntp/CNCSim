#ifndef _EMC_TASK_H_
#define _EMC_TASK_H_

#include "emcMsgManager.h"
#include "canon.hh"
#include "emc.hh"
#include "linuxcnc.h"
#include "tooldata.hh"
#include "interp_base.hh"
#include "mill_task_interface.h"

#define UNEXPECTED_MSG fprintf(stderr,"UNEXPECTED %s %d\n",__FILE__,__LINE__);

struct iocontrol_str {
    bool user_enable_out;     /* output, TRUE when EMC wants stop */
    bool emc_enable_in;        /* input, TRUE on any external stop */
    bool user_request_enable; /* output, used to reset ENABLE latch */
    bool coolant_mist;        /* coolant mist output pin */
    bool coolant_flood;       /* coolant flood output pin */

    // the following pins are needed for toolchanging
    //tool-prepare
    bool tool_prepare;        /* output, pin that notifies HAL it needs to prepare a tool */
    int32_t tool_prep_pocket; /* output, pin that holds the pocketno for the tool table entry matching the tool to be prepared,
                                               only valid when tool-prepare=TRUE */
    int32_t tool_from_pocket; /* output, pin indicating pocket current load tool retrieved from*/
    int32_t tool_prep_index;  /* output, pin for internal index (idx) of prepped tool above */
    int32_t tool_prep_number; /* output, pin that holds the tool number to be prepared, only valid when tool-prepare=TRUE */
    int32_t tool_number;      /* output, pin that holds the tool number currently in the spindle */
    bool tool_prepared;        /* input, pin that notifies that the tool has been prepared */
    //tool-change
    bool tool_change;         /* output, notifies a tool-change should happen (emc should be in the tool-change position) */
    bool tool_changed;         /* input, notifies tool has been changed */

    // note: spindle control has been moved to motion
};                        //pointer to the HAL-struct

#include "motion.h"
#include "motion_struct.h"      /* emcmot_struct_t */
#include <deque>

class EMCTask {
public:
    EMCTask(std::string iniFileName);
    virtual ~EMCTask();

    // Don't copy or assign
    EMCTask(const EMCTask&) = delete;
    EMCTask& operator=(const EMCTask&) = delete;


    int random_toolchanger {0};
    iocontrol_str iocontrol_data;
    const char *ini_filename;
    const char *tooltable_filename {};
    char db_program[LINELEN] {};
    tooldb_t db_mode {tooldb_t::DB_NOTUSED};
    int tool_status;

    int load_file(std::string filename, std::vector<IMillTaskInterface::ToolPath>* toolPath, std::string &err);

    void init_all(void);
private:
    std::string emcFileName_;


    // used to milltask evaluate
    int taskPlanError = 0;
    int taskExecuteError = 0;

    MessageQueue<std::string> emcCmdMsgQueue;
    MessageQueue<std::string> emcStsMsgQueue;
    MessageQueue<std::string> emcStsErrQueue;
    InterpBase *pinterp=0;


    emcmot_command_t *emcmotCommand;
    emcmot_status_t *emcmotStatus;
    emcmot_config_t *emcmotConfig;
    emcmot_internal_t *emcmotInternal;
    emcmot_error_t *emcmotError;
    emcmot_struct_t *emcmotStruct;

    // Arch Helix Generator
    std::deque<IMillTaskInterface::ToolPath> generateG02G03(EmcPose startPose, EmcPose endPos,
                                             PM_CARTESIAN center, PM_CARTESIAN normal,
                                              int type, int turn, int plane, int motion, int segement = 36);

    std::deque<IMillTaskInterface::ToolPath> generateArc(EmcPose startPose, EmcPose endPos,
                                             PM_CARTESIAN center, PM_CARTESIAN normal,int type, int plane, int motion, int segement = 360);

    std::deque<IMillTaskInterface::ToolPath> generateHelix(EmcPose startPose, EmcPose endPos,
                                             PM_CARTESIAN center, PM_CARTESIAN normal);

};


#endif
