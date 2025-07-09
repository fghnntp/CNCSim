#include <iostream>
#include <math.h>		// fabs()
#include <float.h>		// DBL_MAX
#include <string.h>		// memcpy() strncpy()
#include <stdlib.h>		// malloc()
#include <sys/wait.h>

#include "emcTask.h"
#include "rs274ngc_interp.hh"	// the interpreter
#include "emcIniFile.hh"
#include "emcglb.h"
#include "interpl.hh"

#include "emc.hh"



extern void initSimulationUserMotionIF(void);
extern void init_simulate_taskinft(void);

#include "emc_nml.hh"

EMCTask::EMCTask(std::string iniFileName)
    :emcFileName_(iniFileName)
{
    init_all();
//Here Let's start to test interp
//    int code = 0;
//    char errText[256];
//    memset(errText, 0, 256);

//    code = pinterp->execute("G01X10Y10Z10F100") ;
//    printf("code = %d\n", code);
//    if (code >= INTERP_ENDFILE)
//        printf("%s \n", pinterp->error_text(code, errText, 256));
//    interp_list.print();


//    code = pinterp->execute("G01X100Y100Z100F100");
//    if (code >= INTERP_ENDFILE)
//        printf("%s \n", pinterp->error_text(code, errText, 256));
//    printf("code = %d\n", code);
//    interp_list.print();

//    code = pinterp->execute("G01X200Y200Z0F100");
//    if (code >= INTERP_ENDFILE)
//        printf("%s \n", pinterp->error_text(code, errText, 256));
//    printf("code = %d\n", code);
//    interp_list.print();

//    while (interp_list.len() > 0) {
//        auto msg = interp_list.get();
//        if (msg->_type == EMC_TRAJ_LINEAR_MOVE_TYPE) {
//            auto msgRel = msg.get();
//            auto lmmsg = (EMC_TRAJ_LINEAR_MOVE*)msgRel;
//            EmcPose pose = lmmsg->end;
//            printf("x=%f y=%f\n", pose.tran.x, pose.tran.y);

//        }
//    }

}

int EMCTask::load_file(std::string filename, std::vector<IMillTaskInterface::ToolPath>* toolPath)
{
    if (!pinterp)//Wrong
        return 1;

    if (interp_list.len() > 0) {
        //Clear the useless msg first
        interp_list.get();
    }

    if (pinterp->open(filename.c_str())) {
        printf("Wrong filename");
        return 2;
    }

    int code = 0;
    char errText[256];
    int retval = 0;
    memset(errText, 0, 256);
    while (!pinterp->read()) {
        code = pinterp->execute();
        if (code >= INTERP_ENDFILE) {
            retval = 3;
            pinterp->reset();
            printf("%s \n", pinterp->error_text(code, errText, 256));
            break;
        }
    }

    pinterp->close();

    if (retval)
        return retval;

    while (interp_list.len() > 0) {
        auto msg = interp_list.get();
        if (msg->_type == EMC_TRAJ_LINEAR_MOVE_TYPE) {
            auto msgRel = msg.get();
            auto lmmsg = (EMC_TRAJ_LINEAR_MOVE*)msgRel;
            EmcPose pose = lmmsg->end;
            IMillTaskInterface::ToolPath path;
            path.x = pose.tran.x;
            path.y = pose.tran.y;
            path.z = pose.tran.z;
            toolPath->push_back(path);
        }
    }

    return 0;
}

//#include "task.hh"
void EMCTask::init_all()
{
    emcStatus = new EMC_STAT;
    strcpy(emc_inifile, emcFileName_.c_str());

    IniFile inifile;

    if (inifile.Open(emc_inifile)) {
        inifile.Find(&random_toolchanger, "RANDOM_TOOLCHANGER", "EMCIO");
        std::optional<const char*> t;
        if ((t = inifile.Find("TOOL_TABLE", "EMCIO")))
            tooltable_filename = strdup(*t);

        if ((t = inifile.Find("DB_PROGRAM", "EMCIO"))) {
            db_mode = tooldb_t::DB_ACTIVE;
            tooldata_set_db(db_mode);
            strncpy(db_program, *t, LINELEN - 1);
        }

        if (tooltable_filename != NULL && db_program[0] != '\0') {
            fprintf(stderr,"DB_PROGRAM active: IGNORING tool table file %s\n",
                    tooltable_filename);
        }
        inifile.Close();
    }


    tool_mmap_creator((EMC_TOOL_STAT*)&(emcStatus->io.tool), random_toolchanger);
    tool_mmap_user();


    tooldata_init(random_toolchanger);
    if (db_mode == tooldb_t::DB_ACTIVE) {
        if (0 != tooldata_db_init(db_program, random_toolchanger)) {
            printf("can't initialize DB_PROGRAM.\n");
            db_mode = tooldb_t::DB_NOTUSED;
            tooldata_set_db(db_mode);
        }
    }

    emcStatus->io.status = RCS_STATUS::DONE;//TODO??
    emcStatus->io.tool.pocketPrepped = -1;

    if(!random_toolchanger) {
        CANON_TOOL_TABLE tdata = tooldata_entry_init();
        tdata.pocketno =  0; //nonrandom init
        tdata.toolno   = -1; //nonrandom init
        if (tooldata_put(tdata,0) != IDX_OK) {
            UNEXPECTED_MSG;
        }
    }

    if (0 != tooldata_load(tooltable_filename)) {
        printf("can't load tool table.\n");
    }

    if (random_toolchanger) {
        CANON_TOOL_TABLE tdata;
        if (tooldata_get(&tdata,0) != IDX_OK) {
            UNEXPECTED_MSG;//todo: handle error
        }
        emcStatus->io.tool.toolInSpindle = tdata.toolno;
    } else {
        emcStatus->io.tool.toolInSpindle = 0;
    }

    EMC_IO_STAT &emcioStatus = emcStatus->io;

    //Make a rs274 interp

    //Set up rs274 external to know
    emcStatus->motion.traj.axis_mask=~0;

    if(!pinterp) {
        IniFile inifile;
        std::optional<const char*> inistring;
        inifile.Open(emcFileName_.c_str());
        if((inistring = inifile.Find("INTERPRETER", "TASK"))) {
            pinterp = interp_from_shlib(*inistring);
            fprintf(stderr, "interp_from_shlib() -> %p\n", pinterp);
                if (!pinterp) {
                    fprintf(stderr, "failed to load [TASK]INTERPRETER (%s)\n", *inistring);
                }
        }
        inifile.Close();
    }

    if(!pinterp) {
        pinterp = new Interp;
    }

    pinterp->ini_load(emcFileName_.c_str());

    setenv("INI_FILE_NAME", emcFileName_.c_str(), 1);

    int retval = pinterp->init();

    // In task, enable M99 main program endless looping
    pinterp->set_loop_on_main_m99(true);
    if (retval > INTERP_MIN_ERROR) {  // I'd think this should be fatal.

    } else {
        if (0 != rs274ngc_startup_code[0]) {
            retval = pinterp->execute(rs274ngc_startup_code);
            while (retval == INTERP_EXECUTE_FINISH) {
                retval = pinterp->execute(0);
            }
            if (retval > INTERP_MIN_ERROR) {
            }
        }
    }

    EMC_AXIS_STAT state;
    int axis_num_ = 0;

    initSimulationUserMotionIF();
    init_simulate_taskinft();
}

EMCTask::~EMCTask() {}
