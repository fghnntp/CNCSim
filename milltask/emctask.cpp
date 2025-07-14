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
#include <sstream>
#include <iomanip>



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



int EMCTask::load_file(std::string filename, std::vector<IMillTaskInterface::ToolPath>* toolPath, std::string &err)
{
    if (!pinterp)//Wrong
        return 1;

    if (interp_list.len() > 0) {
        //Clear the useless msg first
        interp_list.clear();
        return 2;
    }

    if (pinterp->open(filename.c_str())) {
        std::cout << filename << " Wrong file" << std::endl;
        return 3;
    }

    int code = 0;
    char errText[256];
    memset(errText, 0, 256);
    while (!pinterp->read()) {
        code = pinterp->execute();
        if (code > INTERP_ENDFILE) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(6); // 保证6位小数且非科学计数法
            oss << "file:" << pinterp->file_name(errText, 256);
            oss << " line:" << pinterp->line() << " " << pinterp->line_text(errText, 256);
            oss << " err:" << pinterp->error_text(code, errText, 256);
            err = oss.str();
            std::cout << err << std::endl;
            pinterp->reset();//This should reset the interp
            pinterp->close();
            return 4;
        }
    }

    pinterp->close();

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
        else if (msg->_type == EMC_TRAJ_CIRCULAR_MOVE_TYPE) {
            auto msgRel = msg.get();
            auto lmmsg = (EMC_TRAJ_CIRCULAR_MOVE*)msgRel;
            EmcPose startPos;
            EmcPose endPos;
            PM_CARTESIAN center;
            PM_CARTESIAN normal;
            int turn;
            int type;

            startPos.tran.x = 0.0;
            startPos.tran.y = 0.0;
            startPos.tran.z = 0.0;
            if (toolPath->size() > 0) {
                IMillTaskInterface::ToolPath lastPath = toolPath->back();
                startPos.tran.x = lastPath.x;
                startPos.tran.y = lastPath.y;
                startPos.tran.z = lastPath.z;
            }

            endPos = lmmsg->end;
            center = lmmsg->center;
            normal = lmmsg->normal;
            turn = lmmsg->turn;
            type = lmmsg->type;
            int plane = lmmsg->tag.get_state_tag().fields[GM_FIELD_PLANE];
            int motion = lmmsg->tag.get_state_tag().fields[GM_FIELD_MOTION_MODE];

            auto G0203Path = generateG02G03(startPos, endPos, center, normal, type, turn, plane, motion);
            while (!G0203Path.empty()) {
                auto elemnt = G0203Path.front();
                toolPath->push_back(elemnt);
                G0203Path.pop_front();
            }
        }
    }

    if (toolPath->size() == 0)
        return 5;

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

    // This is motion simulation init and task simulation init
    initSimulationUserMotionIF();
    init_simulate_taskinft();
}

std::deque <IMillTaskInterface::ToolPath>
EMCTask::generateG02G03(EmcPose startPose, EmcPose endPos,
                      PM_CARTESIAN center, PM_CARTESIAN normal,
                      int type, int turn, int plane, int motion, int segement)
{
    std::deque<IMillTaskInterface::ToolPath> points;

    if (sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z) < 0.001
            || (motion != G_2 && motion != G_3)) {
        return points;
    }

    if (turn > 0) {
        return points;
    }
    else {
        points = generateArc(startPose, endPos, center, normal, type, plane, motion);
    }

    return points;
}

std::deque<IMillTaskInterface::ToolPath>
EMCTask::generateArc(EmcPose startPose, EmcPose endPose, PM_CARTESIAN center, PM_CARTESIAN normal, int type, int plane, int motion,
                     int segement)
{
    std::deque<IMillTaskInterface::ToolPath> points;

    IMillTaskInterface::ToolPath startPoint;
    IMillTaskInterface::ToolPath endPoint;
    IMillTaskInterface::ToolPath centerPoint;

    startPoint.x = startPose.tran.x;
    startPoint.y = startPose.tran.y;
    startPoint.z = startPose.tran.z;

    endPoint.x = endPose.tran.x;
    endPoint.y = endPose.tran.y;
    endPoint.z = endPose.tran.z;

    centerPoint.x = center.x;
    centerPoint.y = center.y;
    centerPoint.z = center.z;

    IMillTaskInterface::ToolPath startVec;
    IMillTaskInterface::ToolPath endVec;

    startVec.x = startPoint.x - centerPoint.x;
    startVec.y = startPoint.y - centerPoint.y;
    startVec.z = startPoint.z - centerPoint.z;

    endVec.x = endPoint.x - centerPoint.x;
    endVec.y = endPoint.y - centerPoint.y;
    endVec.z = endPoint.z - centerPoint.z;

    float radius = sqrt(startVec.x * startVec.x
                        + startVec.y * startVec.y
                        + startVec.z * startVec.z);

    if (fabs(radius) < 0.001)//This is not move traject
        return points;

    if (plane == G_19) {
        //YZ plane select
        double startAngle = atan2(startVec.z, startVec.y);
        double endAngle = atan2(endVec.z, endVec.y);

        double stepAngle = 2.0 * M_PI / segement;
        int cnt = 0;
        double angle = 0.0;

        if (fabs(startAngle - endAngle) < 0.001) {

        }
        else {
            segement = (fabs(endAngle - startAngle) / (2.0 * M_PI)) * segement;
        }

        while (cnt <= segement) {
            points.push_back(IMillTaskInterface::ToolPath{startPoint.x,
                                                          center.y + cos(angle),
                                                          center.z + sin(angle)});

            if (motion == G_2) {
                angle -= stepAngle;
            }
            else if (motion == G_3) {
                angle += stepAngle;
            }
            cnt += 1;
        }

    }
    else if (plane == G_18) {
        //XZ plane select
        double startAngle = atan2(startVec.z, startVec.x);
        double endAngle = atan2(endVec.z, endVec.x);

        double stepAngle = 2.0 * M_PI / segement;
        int cnt = 0;
        double angle = 0.0;

        if (fabs(startAngle - endAngle) < 0.001) {

        }
        else {
            segement = (fabs(endAngle - startAngle) / (2.0 * M_PI)) * segement;
        }

        while (cnt <= segement) {
            points.push_back(IMillTaskInterface::ToolPath{center.y + cos(angle),
                                                          0.0,
                                                          center.z + sin(angle)});

            if (motion == G_2) {
                angle -= stepAngle;
            }
            else if (motion == G_3) {
                angle += stepAngle;
            }
            cnt += 1;
        }
    }
    else if (plane == G_17) {
        //XY plane select
        double startAngle = atan2(startVec.y, startVec.x);
        double endAngle = atan2(endVec.y, endVec.x);
        double stepAngle = 2.0 * M_PI / segement;
        int cnt = 0;
        double angle = 0.0;

        if (fabs(startAngle - endAngle) < 0.001) {

        }
        else {
            segement = (fabs(endAngle - startAngle) / (2.0 * M_PI)) * segement;
        }

        while (cnt <= segement) {
            points.push_back(IMillTaskInterface::ToolPath{center.x + cos(angle),
                                                          center.y + sin(angle),
                                                          startPoint.z});

            if (motion == G_2) {
                angle -= stepAngle;
            }
            else if (motion == G_3) {
                angle += stepAngle;
            }
            cnt += 1;
        }
    }

    return points;
}

EMCTask::~EMCTask() {}
