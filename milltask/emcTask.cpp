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

#include "emcParas.h"
#include "emc_nml.hh"

EMCTask::EMCTask(std::string iniFileName)
    :emcFileName_(iniFileName)
{
    EMCParas::inifileName = emcFileName_;
    init_all();
//Here Let's start to test interp
    int code = 0;
    char errText[256];
    memset(errText, 0, 256);

    code = pinterp->execute("G01X10Y10Z10F100") ;
    printf("code = %d\n", code);
    if (code >= INTERP_ENDFILE)
        printf("%s \n", pinterp->error_text(code, errText, 256));
    interp_list.print();


    code = pinterp->execute("G01X100Y100Z100F100");
    if (code >= INTERP_ENDFILE)
        printf("%s \n", pinterp->error_text(code, errText, 256));
    printf("code = %d\n", code);
    interp_list.print();

    code = pinterp->execute("G01X200Y200Z0F100");
    if (code >= INTERP_ENDFILE)
        printf("%s \n", pinterp->error_text(code, errText, 256));
    printf("code = %d\n", code);
    interp_list.print();

    while (interp_list.len() > 0) {
        auto msg = interp_list.get();
        if (msg->_type == EMC_TRAJ_LINEAR_MOVE_TYPE) {
            auto msgRel = msg.get();
            auto lmmsg = (EMC_TRAJ_LINEAR_MOVE*)msgRel;
            EmcPose pose = lmmsg->end;
            printf("x=%f y=%f\n", pose.tran.x, pose.tran.y);

        }
    }

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

#include "motionTask.h"
void EMCTask::init_all()
{
    //init milltask moudle
    //rs274ngc_parser python_interpter(inited by canon and parser) tool
    emcStatus = new EMC_STAT;

    if (emcFileName_ != "")
        strncpy(emc_inifile, emcFileName_.c_str(), LINELEN);

    IniFile inifile;

    if (inifile.Open(emc_inifile)) {
        inifile.Find(&random_toolchanger, "RANDOM_TOOLCHANGER", "EMCIO");
        std::optional<const char*> t;
        if ((t = inifile.Find("TOOL_TABLE", "EMCIO")))
            tooltable_filename = strdup(*t);
        inifile.Close();
    }

    tool_mmap_creator((EMC_TOOL_STAT*)&(emcStatus->io.tool), random_toolchanger);
    tool_mmap_user();


    tooldata_init(random_toolchanger);

    emcStatus->io.status = RCS_STATUS::DONE;//TODO??
    emcStatus->io.tool.pocketPrepped = -1;

    if(!random_toolchanger) {
        CANON_TOOL_TABLE tdata = tooldata_entry_init();
        tdata.pocketno =  0; //nonrandom init
        tdata.toolno   = -1; //nonrandom init
        if (tooldata_put(tdata,0) != IDX_OK) {
            printf("random_toolchanger error.\n");
        }
    }

    if (0 != tooldata_load(tooltable_filename)) {
        printf("can't load tool table.\n");
    }

    if (random_toolchanger) {
        CANON_TOOL_TABLE tdata;
        if (tooldata_get(&tdata,0) != IDX_OK) {
            printf("random_toolchanger error.\n");
        }
        emcStatus->io.tool.toolInSpindle = tdata.toolno;
    } else {
        emcStatus->io.tool.toolInSpindle = 0;
    }

    //Make a rs274 interp

    //Set up rs274 external to know
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

    EMCParas::init_emc_paras();
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

    MotionTask::InitMotion();

    EMC_TRAJ_SET_SCALE testmsg;
    testmsg.scale = 0.8;
    emcTaskIssueTrajCmd(&testmsg);
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

    if ( (plane == G_17 && fabs(normal.z) > 0.001 )
         || (plane == G_18 && fabs(normal.y) > 0.001 )
         || (plane == G_19 && fabs(normal.x) > 0.001 ) ) {
        points = generateHelix(startPose, endPos, center, normal, type, turn, plane, motion);
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

std::deque<IMillTaskInterface::ToolPath>
EMCTask::generateHelix(EmcPose startPose, EmcPose endPose, PM_CARTESIAN center, PM_CARTESIAN normal, int type, int turn, int plane, int motion,
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

    if (turn == 0)
        turn = 1;

    if (plane == G_19) {
        //YZ plane select

        float radius = sqrt(startVec.y * startVec.y
                            + startVec.z * startVec.z);

        if (fabs(radius) < 0.001)//This is not move traject
            return points;

        double stepAngle = 2.0 * M_PI / segement;
        int cnt = 0;
        double angle = 0.0;
        double pitch = 0.0;
        double endP = startPoint.x;

        segement = segement * turn;
        pitch = (endPoint.x - startPoint.x) / segement;

        while (cnt <= segement) {
            points.push_back(IMillTaskInterface::ToolPath{endP,
                                                          center.y + cos(angle),
                                                          center.z + sin(angle)});
            endP += pitch;

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
        float radius = sqrt(startVec.x * startVec.x
                            + startVec.z * startVec.z);

        if (fabs(radius) < 0.001)//This is not move traject
            return points;

        double stepAngle = 2.0 * M_PI / segement;
        int cnt = 0;
        double angle = 0.0;
        double pitch = 0.0;
        double endP = startPoint.y;

        segement = segement * turn;
        pitch = (endPoint.y - startPoint.y) / segement;

        while (cnt <= segement) {
            points.push_back(IMillTaskInterface::ToolPath{center.x + cos(angle),
                                                          endP,
                                                          center.z + sin(angle)});
            endP += pitch;

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
        float radius = sqrt(startVec.x * startVec.x
                            + startVec.y * startVec.y);

        if (fabs(radius) < 0.001)//This is not move traject
            return points;

        double stepAngle = 2.0 * M_PI / segement;
        int cnt = 0;
        double angle = 0.0;
        double pitch = 0.0;
        double endP = startPoint.z;

        segement = segement * turn;
        pitch = (endPoint.z - startPoint.z) / segement;


        while (cnt <= segement) {
            points.push_back(IMillTaskInterface::ToolPath{center.x + cos(angle),
                                                          center.y + sin(angle),
                                                          endP});

            endP += pitch;

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

#include "emcChannel.h"

int EMCTask::emcTaskIssueTrajCmd(NMLmsg *cmd)
{
    int retval = 0;
    int execRetval = 0;
    static char remote_tmpfilename[LINELEN];   // path to temporary file received from remote process

    switch (cmd->_type) {
    // general commands

    // traj commands

    case EMC_TRAJ_SET_SCALE_TYPE:
        EMCChannel::emcTrajSetScaleMsg = (EMC_TRAJ_SET_SCALE *) cmd;
        retval = EMCChannel::emcTrajSetScale(EMCChannel::emcTrajSetScaleMsg->scale);
    break;

    case EMC_TRAJ_SET_RAPID_SCALE_TYPE:
        EMCChannel::emcTrajSetRapidScaleMsg = (EMC_TRAJ_SET_RAPID_SCALE *) cmd;
        retval = EMCChannel::emcTrajSetRapidScale(EMCChannel::emcTrajSetRapidScaleMsg->scale);
    break;

    case EMC_TRAJ_SET_MAX_VELOCITY_TYPE:
        EMCChannel::emcTrajSetMaxVelocityMsg = (EMC_TRAJ_SET_MAX_VELOCITY *) cmd;
        retval = EMCChannel::emcTrajSetMaxVelocity(EMCChannel::emcTrajSetMaxVelocityMsg->velocity);
    break;

    case EMC_TRAJ_SET_SPINDLE_SCALE_TYPE:
//    emcTrajSetSpindleScaleMsg = (EMC_TRAJ_SET_SPINDLE_SCALE *) cmd;
//    retval = emcTrajSetSpindleScale(emcTrajSetSpindleScaleMsg->spindle,
//                                    emcTrajSetSpindleScaleMsg->scale);
    break;

    case EMC_TRAJ_SET_FO_ENABLE_TYPE:
//    retval = emcTrajSetFOEnable(((EMC_TRAJ_SET_FO_ENABLE *) cmd)->mode);  // feed override enable/disable
    break;

    case EMC_TRAJ_SET_FH_ENABLE_TYPE:
//    retval = emcTrajSetFHEnable(((EMC_TRAJ_SET_FH_ENABLE *) cmd)->mode); //feed hold enable/disable
    break;

    case EMC_TRAJ_SET_SO_ENABLE_TYPE:
//    retval = emcTrajSetSOEnable(((EMC_TRAJ_SET_SO_ENABLE *) cmd)->mode); //spindle speed override enable/disable
    break;

    case EMC_TRAJ_SET_VELOCITY_TYPE:
//    emcTrajSetVelocityMsg = (EMC_TRAJ_SET_VELOCITY *) cmd;
//    retval = emcTrajSetVelocity(emcTrajSetVelocityMsg->velocity,
//            emcTrajSetVelocityMsg->ini_maxvel);
    break;

    case EMC_TRAJ_SET_ACCELERATION_TYPE:
//    emcTrajSetAccelerationMsg = (EMC_TRAJ_SET_ACCELERATION *) cmd;
//    retval = emcTrajSetAcceleration(emcTrajSetAccelerationMsg->acceleration);
    break;

    case EMC_TRAJ_LINEAR_MOVE_TYPE:
//    emcTrajUpdateTag(((EMC_TRAJ_LINEAR_MOVE *) cmd)->tag);
//    emcTrajLinearMoveMsg = (EMC_TRAJ_LINEAR_MOVE *) cmd;
//        retval = emcTrajLinearMove(emcTrajLinearMoveMsg->end,
//                                   emcTrajLinearMoveMsg->type, emcTrajLinearMoveMsg->vel,
//                                   emcTrajLinearMoveMsg->ini_maxvel, emcTrajLinearMoveMsg->acc,
//                                   emcTrajLinearMoveMsg->indexer_jnum);
    break;

    case EMC_TRAJ_CIRCULAR_MOVE_TYPE:
//    emcTrajUpdateTag(((EMC_TRAJ_LINEAR_MOVE *) cmd)->tag);
//    emcTrajCircularMoveMsg = (EMC_TRAJ_CIRCULAR_MOVE *) cmd;
//        retval = emcTrajCircularMove(emcTrajCircularMoveMsg->end,
//                emcTrajCircularMoveMsg->center, emcTrajCircularMoveMsg->normal,
//                emcTrajCircularMoveMsg->turn, emcTrajCircularMoveMsg->type,
//                emcTrajCircularMoveMsg->vel,
//                emcTrajCircularMoveMsg->ini_maxvel,
//                emcTrajCircularMoveMsg->acc);
    break;

    case EMC_TRAJ_PAUSE_TYPE:
//    emcStatus->task.task_paused = 1;
    retval = emcTrajPause();
    break;

    case EMC_TRAJ_RESUME_TYPE:
//    emcStatus->task.task_paused = 0;
//    retval = emcTrajResume();
    break;

    case EMC_TRAJ_ABORT_TYPE:
//    retval = emcTrajAbort();
    break;

    case EMC_TRAJ_DELAY_TYPE:
//    emcTrajDelayMsg = (EMC_TRAJ_DELAY *) cmd;
    // set the timeout clock to expire at 'now' + delay time
//    taskExecDelayTimeout = etime() + emcTrajDelayMsg->delay;
    retval = 0;
    break;

    case EMC_TRAJ_SET_TERM_COND_TYPE:
//    emcTrajSetTermCondMsg = (EMC_TRAJ_SET_TERM_COND *) cmd;
//    retval = emcTrajSetTermCond(emcTrajSetTermCondMsg->cond, emcTrajSetTermCondMsg->tolerance);
    break;

    case EMC_TRAJ_SET_SPINDLESYNC_TYPE:
//        emcTrajSetSpindlesyncMsg = (EMC_TRAJ_SET_SPINDLESYNC *) cmd;
//        retval = emcTrajSetSpindleSync(emcTrajSetSpindlesyncMsg->spindle, emcTrajSetSpindlesyncMsg->feed_per_revolution, emcTrajSetSpindlesyncMsg->velocity_mode);
        break;

    case EMC_TRAJ_SET_OFFSET_TYPE:
    // update tool offset
    emcStatus->task.toolOffset = ((EMC_TRAJ_SET_OFFSET *) cmd)->offset;
        retval = emcTrajSetOffset(emcStatus->task.toolOffset);
    break;

    case EMC_TRAJ_SET_ROTATION_TYPE:
        emcStatus->task.rotation_xy = ((EMC_TRAJ_SET_ROTATION *) cmd)->rotation;
        retval = 0;
        break;

    case EMC_TRAJ_SET_G5X_TYPE:
    // struct-copy program origin
    emcStatus->task.g5x_offset = ((EMC_TRAJ_SET_G5X *) cmd)->origin;
        emcStatus->task.g5x_index = ((EMC_TRAJ_SET_G5X *) cmd)->g5x_index;
    retval = 0;
    break;
    case EMC_TRAJ_SET_G92_TYPE:
    // struct-copy program origin
    emcStatus->task.g92_offset = ((EMC_TRAJ_SET_G92 *) cmd)->origin;
    retval = 0;
    break;
    case EMC_TRAJ_CLEAR_PROBE_TRIPPED_FLAG_TYPE:
    retval = emcTrajClearProbeTrippedFlag();
    break;

    case EMC_TRAJ_PROBE_TYPE:
    retval = emcTrajProbe(
        ((EMC_TRAJ_PROBE *) cmd)->pos,
        ((EMC_TRAJ_PROBE *) cmd)->type,
        ((EMC_TRAJ_PROBE *) cmd)->vel,
            ((EMC_TRAJ_PROBE *) cmd)->ini_maxvel,
        ((EMC_TRAJ_PROBE *) cmd)->acc,
            ((EMC_TRAJ_PROBE *) cmd)->probe_type);
    break;

    case EMC_AUX_INPUT_WAIT_TYPE:
//    emcAuxInputWaitMsg = (EMC_AUX_INPUT_WAIT *) cmd;
//    if (emcAuxInputWaitMsg->timeout == WAIT_MODE_IMMEDIATE) { //nothing to do, CANON will get the needed value when asked by the interp
//        emcStatus->task.input_timeout = 0; // no timeout can occur
//        emcAuxInputWaitIndex = -1;
//        taskExecDelayTimeout = 0.0;
//    } else {
//        emcAuxInputWaitType = emcAuxInputWaitMsg->wait_type; // remember what we are waiting for
//        emcAuxInputWaitIndex = emcAuxInputWaitMsg->index; // remember the input to look at
//        emcStatus->task.input_timeout = 2; // set timeout flag, gets cleared if input changes before timeout happens
//        // set the timeout clock to expire at 'now' + delay time
//        taskExecDelayTimeout = etime() + emcAuxInputWaitMsg->timeout;
//    }
    break;

    case EMC_SPINDLE_WAIT_ORIENT_COMPLETE_TYPE:
//    wait_spindle_orient_complete_msg = (EMC_SPINDLE_WAIT_ORIENT_COMPLETE *) cmd;
//    taskExecDelayTimeout = etime() + wait_spindle_orient_complete_msg->timeout;
    break;

    case EMC_TRAJ_RIGID_TAP_TYPE:
    emcTrajUpdateTag(((EMC_TRAJ_LINEAR_MOVE *) cmd)->tag);
    retval = emcTrajRigidTap(((EMC_TRAJ_RIGID_TAP *) cmd)->pos,
            ((EMC_TRAJ_RIGID_TAP *) cmd)->vel,
            ((EMC_TRAJ_RIGID_TAP *) cmd)->ini_maxvel,
        ((EMC_TRAJ_RIGID_TAP *) cmd)->acc,
        ((EMC_TRAJ_RIGID_TAP *) cmd)->scale);
    break;

    case EMC_TRAJ_SET_TELEOP_ENABLE_TYPE:
    if (((EMC_TRAJ_SET_TELEOP_ENABLE *) cmd)->enable) {
        retval = emcTrajSetMode(EMC_TRAJ_MODE::TELEOP);
    } else {
        retval = emcTrajSetMode(EMC_TRAJ_MODE::FREE);
    }
    break;

    case EMC_MOTION_SET_AOUT_TYPE:
    retval = emcMotionSetAout(((EMC_MOTION_SET_AOUT *) cmd)->index,
                  ((EMC_MOTION_SET_AOUT *) cmd)->start,
                  ((EMC_MOTION_SET_AOUT *) cmd)->end,
                  ((EMC_MOTION_SET_AOUT *) cmd)->now);
    break;

    case EMC_MOTION_SET_DOUT_TYPE:
    retval = emcMotionSetDout(((EMC_MOTION_SET_DOUT *) cmd)->index,
                  ((EMC_MOTION_SET_DOUT *) cmd)->start,
                  ((EMC_MOTION_SET_DOUT *) cmd)->end,
                  ((EMC_MOTION_SET_DOUT *) cmd)->now);
    break;

    case EMC_MOTION_ADAPTIVE_TYPE:
    retval = emcTrajSetAFEnable(((EMC_MOTION_ADAPTIVE *) cmd)->status);
    break;

    case EMC_SET_DEBUG_TYPE:
    /* set the debug level here */
    emc_debug = ((EMC_SET_DEBUG *) cmd)->debug;
    /* and motion */
    emcMotionSetDebug(emc_debug);
    /* and reflect it in the status-- this isn't updated continually */
    emcStatus->debug = emc_debug;
    break;

    // unimplemented ones

    // IO commands

    case EMC_SPINDLE_SPEED_TYPE:
//    spindle_speed_msg = (EMC_SPINDLE_SPEED *) cmd;
//    retval = emcSpindleSpeed(spindle_speed_msg->spindle, spindle_speed_msg->speed,
//            spindle_speed_msg->factor, spindle_speed_msg->xoffset);
    break;

    case EMC_SPINDLE_ORIENT_TYPE:
//    spindle_orient_msg = (EMC_SPINDLE_ORIENT *) cmd;
//    retval = emcSpindleOrient(spindle_orient_msg->spindle, spindle_orient_msg->orientation,
//            spindle_orient_msg->mode);
    break;

    case EMC_SPINDLE_ON_TYPE:
//    spindle_on_msg = (EMC_SPINDLE_ON *) cmd;
//    retval = emcSpindleOn(spindle_on_msg->spindle, spindle_on_msg->speed,
//            spindle_on_msg->factor, spindle_on_msg->xoffset, spindle_on_msg->wait_for_spindle_at_speed);
    break;

    case EMC_SPINDLE_OFF_TYPE:
//    spindle_off_msg = (EMC_SPINDLE_OFF *) cmd;
//    retval = emcSpindleOff(spindle_off_msg->spindle);
    break;

    case EMC_SPINDLE_BRAKE_RELEASE_TYPE:
//    spindle_brake_release_msg = (EMC_SPINDLE_BRAKE_RELEASE *) cmd;
//    retval = emcSpindleBrakeRelease(spindle_brake_release_msg->spindle);
    break;

    case EMC_SPINDLE_INCREASE_TYPE:
//    spindle_increase_msg = (EMC_SPINDLE_INCREASE *) cmd;
//    retval = emcSpindleIncrease(spindle_increase_msg->spindle);
    break;

    case EMC_SPINDLE_DECREASE_TYPE:
//    spindle_decrease_msg = (EMC_SPINDLE_DECREASE *) cmd;
//    retval = emcSpindleDecrease(spindle_decrease_msg->spindle);
    break;

    case EMC_SPINDLE_CONSTANT_TYPE:
//    spindle_constant_msg = (EMC_SPINDLE_CONSTANT *) cmd;
//    retval = emcSpindleConstant(spindle_constant_msg->spindle);
    break;

    case EMC_SPINDLE_BRAKE_ENGAGE_TYPE:
//    spindle_brake_engage_msg = (EMC_SPINDLE_BRAKE_ENGAGE *) cmd;
//    retval = emcSpindleBrakeEngage(spindle_brake_engage_msg->spindle);
    break;

    case EMC_COOLANT_MIST_ON_TYPE:
    retval = emcCoolantMistOn();
    break;

    case EMC_COOLANT_MIST_OFF_TYPE:
    retval = emcCoolantMistOff();
    break;

    case EMC_COOLANT_FLOOD_ON_TYPE:
    retval = emcCoolantFloodOn();
    break;

    case EMC_COOLANT_FLOOD_OFF_TYPE:
    retval = emcCoolantFloodOff();
    break;

    case EMC_TOOL_PREPARE_TYPE:
//    tool_prepare_msg = (EMC_TOOL_PREPARE *) cmd;
//    retval = emcToolPrepare(tool_prepare_msg->tool);
    break;

    case EMC_TOOL_LOAD_TYPE:
//    retval = emcToolLoad();
    break;

    case EMC_TOOL_UNLOAD_TYPE:
//    retval = emcToolUnload();
    break;

    case EMC_TOOL_LOAD_TOOL_TABLE_TYPE:
//    load_tool_table_msg = (EMC_TOOL_LOAD_TOOL_TABLE *) cmd;
//    retval = emcToolLoadToolTable(load_tool_table_msg->file);
    break;

    case EMC_TOOL_SET_OFFSET_TYPE:
//    emc_tool_set_offset_msg = (EMC_TOOL_SET_OFFSET *) cmd;
//    retval = emcToolSetOffset(emc_tool_set_offset_msg->pocket,
//                                  emc_tool_set_offset_msg->toolno,
//                                  emc_tool_set_offset_msg->offset,
//                                  emc_tool_set_offset_msg->diameter,
//                                  emc_tool_set_offset_msg->frontangle,
//                                  emc_tool_set_offset_msg->backangle,
//                                  emc_tool_set_offset_msg->orientation);
//    break;

//    case EMC_TOOL_SET_NUMBER_TYPE:
//    emc_tool_set_number_msg = (EMC_TOOL_SET_NUMBER *) cmd;
//    retval = emcToolSetNumber(emc_tool_set_number_msg->tool);
    break;

    // task commands

    case EMC_TASK_ABORT_TYPE:
//    // abort everything
//    emcTaskAbort();
//    // KLUDGE call motion abort before state restore to make absolutely sure no
//    // stray restore commands make it down to motion
//    emcMotionAbort();
//    // Then call state restore to update the interpreter
//    emcTaskStateRestore();
//        emcIoAbort(EMC_ABORT::TASK_ABORT);
//    for (int s = 0; s < emcStatus->motion.traj.spindles; s++) emcSpindleAbort(s);
//    mdi_execute_abort();
//    emcAbortCleanup(EMC_ABORT::TASK_ABORT);
//    retval = 0;
    break;

    // mode and state commands

    case EMC_TASK_SET_MODE_TYPE:
//    mode_msg = (EMC_TASK_SET_MODE *) cmd;
//    if (emcStatus->task.mode == EMC_TASK_MODE::AUTO &&
//        emcStatus->task.interpState != EMC_TASK_INTERP::IDLE &&
//        mode_msg->mode != EMC_TASK_MODE::AUTO) {
//        emcOperatorError(_("Can't switch mode while mode is AUTO and interpreter is not IDLE"));
//    } else { // we can honour the modeswitch
//        if (mode_msg->mode == EMC_TASK_MODE::MANUAL &&
//        emcStatus->task.mode != EMC_TASK_MODE::MANUAL) {
//        // leaving auto or mdi mode for manual

//        /*! \todo FIXME-- duplicate code for abort,
//            also near end of main, when aborting on subordinate errors,
//            and in emcTaskExecute() */

//        // abort motion
//        emcTaskAbort();
//        mdi_execute_abort();

//        // without emcTaskPlanClose(), a new run command resumes at
//        // aborted line-- feature that may be considered later
//        {
//            int was_open = taskplanopen;
//            emcTaskPlanClose();
//            if (emc_debug & EMC_DEBUG_INTERP && was_open) {
//            rcs_print("emcTaskPlanClose() called at %s:%d\n",
//                  __FILE__, __LINE__);
//            }
//        }

//        // clear out the pending command
//        emcTaskCommand = 0;
//        interp_list.clear();
//                emcStatus->task.currentLine = 0;

//        // clear out the interpreter state
//        emcStatus->task.interpState = EMC_TASK_INTERP::IDLE;
//        emcStatus->task.execState = EMC_TASK_EXEC::DONE;
//        stepping = 0;
//        steppingWait = 0;

//        // now queue up command to resynch interpreter
//        emcTaskQueueTaskPlanSynchCmd();
//        }
//        retval = emcTaskSetMode(mode_msg->mode);
//    }
    break;

    case EMC_TASK_SET_STATE_TYPE:
//    state_msg = (EMC_TASK_SET_STATE *) cmd;
//    retval = emcTaskSetState(state_msg->state);
    break;

    // interpreter commands

    case EMC_TASK_PLAN_CLOSE_TYPE:
//        retval = emcTaskPlanClose();
//    if (retval > INTERP_MIN_ERROR) {
//        emcOperatorError(_("failed to close file"));
//        retval = -1;
//    } else {
//        retval = 0;
//        }
//        /* delete temporary copy of remote file if it exists */
//    if(remote_tmpfilename[0])
//            unlink(remote_tmpfilename);
//        break;

    case EMC_TASK_PLAN_OPEN_TYPE:
//        open_msg = (EMC_TASK_PLAN_OPEN *) cmd;

//        /* receive file in chunks from remote process via open_msg->remote_buffer */
//        if(open_msg->remote_filesize > 0) {
//            static size_t received;             // amount of bytes of file received, yet
//            static int fd;                      // temporary file

//            /* got empty chunk? */
//        if(open_msg->remote_buffersize == 0) {
//        rcs_print_error("EMC_TASK_PLAN_OPEN received empty chunk from remote process.\n");
//        retval = -1;
//        break;
//        }

//        /* this chunk belongs to a new file? */
//        if(received == 0) {
//                /* create new tempfile */
//        snprintf(remote_tmpfilename, LINELEN, EMC2_TMP_DIR "/%s.XXXXXX", basename(open_msg->file));
//        if((fd = mkstemp(remote_tmpfilename)) < 0) {
//                    rcs_print_error("mkstemp(%s) error: %s", remote_tmpfilename, strerror(errno));
//            retval = -1;
//            break;
//        }
//        }

//        /* write chunk to tempfile */
//            size_t bytes_written = write(fd, open_msg->remote_buffer, open_msg->remote_buffersize);
//        if(bytes_written < open_msg->remote_buffersize) {
//        rcs_print_error("fwrite(%s) error: %s", remote_tmpfilename, strerror(errno));
//        retval = -1;
//        break;
//        }
//        /* record data written */
//        received += bytes_written;

//        /* not the last chunk? */
//        if(received < open_msg->remote_filesize) {
//        /* we're done here for now */
//        retval = 0;
//        break;
//        }
//        /* all chunks received - reset byte counter */
//        received = 0;
//        /* close file */
//        close(fd);
//        /* change filename to newly written local tmp file */
//        rtapi_strxcpy(open_msg->file, remote_tmpfilename);
//    }

//    /* open local file */
//    retval = emcTaskPlanOpen(open_msg->file);
//    if (retval > INTERP_MIN_ERROR) {
//        retval = -1;
//    }
//    if (-1 == retval) {
//        emcOperatorError(_("can't open %s"), open_msg->file);
//    } else {
//        rtapi_strxcpy(emcStatus->task.file, open_msg->file);
//        retval = 0;
//    }
    break;

    case EMC_TASK_PLAN_EXECUTE_TYPE:
//    stepping = 0;
//    steppingWait = 0;
//    execute_msg = (EMC_TASK_PLAN_EXECUTE *) cmd;
//        if (!all_homed() && !no_force_homing) { //!no_force_homing = force homing before MDI
//            emcOperatorError(_("Can't issue MDI command when not homed"));
//            retval = -1;
//            break;
//        }
//        if (emcStatus->task.mode != EMC_TASK_MODE::MDI) {
//            emcOperatorError(_("Must be in MDI mode to issue MDI command"));
//            retval = -1;
//            break;
//        }
//    // track interpState also during MDI - it might be an oword sub call
//    emcStatus->task.interpState = EMC_TASK_INTERP::READING;

//    if (execute_msg->command[0] != 0) {
//        char * command = execute_msg->command;
//        if (command[0] == (char) 0xff) {
//        // Empty command received. Consider it is NULL
//        command = NULL;
//        } else {
//        // record initial MDI command
//        rtapi_strxcpy(emcStatus->task.command, execute_msg->command);
//        }

//        int level = emcTaskPlanLevel();
//        if (emcStatus->task.mode == EMC_TASK_MODE::MDI) {
//        if (mdi_execute_level < 0)
//            mdi_execute_level = level;
//        }

//        execRetval = emcTaskPlanExecute(command, 0);

//        level = emcTaskPlanLevel();

//        if (emcStatus->task.mode == EMC_TASK_MODE::MDI) {
//        if (mdi_execute_level == level) {
//            mdi_execute_level = -1;
//        } else if (level > 0) {
//            // Still insude call. Need another execute(0) call
//            // but only if we didn't encounter an error
//            if (execRetval == INTERP_ERROR) {
//            mdi_execute_next = 0;
//            } else {
//            mdi_execute_next = 1;
//            }
//        }
//        }
//        switch (execRetval) {

//        case INTERP_EXECUTE_FINISH:
//        // Flag MDI wait
//        mdi_execute_wait = 1;
//        // need to flush execution, so signify no more reading
//        // until all is done
//        emcTaskPlanSetWait();
//        // and resynch the interpreter WM
//        emcTaskQueueTaskPlanSynchCmd();
//        // it's success, so retval really is 0
//        retval = 0;
//        break;

//        case INTERP_ERROR:
//        // emcStatus->task.interpState =  EMC_TASK_INTERP::WAITING;
//        interp_list.clear();
//        // abort everything
//        emcTaskAbort();
//        emcIoAbort(EMC_ABORT::INTERPRETER_ERROR_MDI);
//        for (int s = 0; s < emcStatus->motion.traj.spindles; s++) emcSpindleAbort(s);
//        mdi_execute_abort(); // sets emcStatus->task.interpState to  EMC_TASK_INTERP::IDLE
//        emcAbortCleanup(EMC_ABORT::INTERPRETER_ERROR_MDI, "interpreter error during MDI");
//        retval = -1;
//        break;

//        case INTERP_EXIT:
//        case INTERP_ENDFILE:
//        case INTERP_FILE_NOT_OPEN:
//        // this caused the error msg on M2 in MDI mode - execRetval == INTERP_EXIT which is would be ok (I think). mah
//        retval = -1;
//        break;

//        default:
//        // other codes are OK
//        retval = 0;
//        }
//    }
    break;

    case EMC_TASK_PLAN_RUN_TYPE:
//        if (!all_homed() && !no_force_homing) { //!no_force_homing = force homing before Auto
//            emcOperatorError(_("Can't run a program when not homed"));
//            retval = -1;
//            break;
//        }
//    stepping = 0;
//    steppingWait = 0;
//    if (!taskplanopen && emcStatus->task.file[0] != 0) {
//        emcTaskPlanOpen(emcStatus->task.file);
//    }
//    run_msg = (EMC_TASK_PLAN_RUN *) cmd;
//    programStartLine = run_msg->line;
//    emcStatus->task.interpState = EMC_TASK_INTERP::READING;
//    emcStatus->task.task_paused = 0;
//    retval = 0;
    break;

    case EMC_TASK_PLAN_PAUSE_TYPE:
//    emcTrajPause();
//    if (emcStatus->task.interpState != EMC_TASK_INTERP::PAUSED) {
//        interpResumeState = emcStatus->task.interpState;
//    }
//    emcStatus->task.interpState = EMC_TASK_INTERP::PAUSED;
//    emcStatus->task.task_paused = 1;
//    retval = 0;
    break;

    case EMC_TASK_PLAN_OPTIONAL_STOP_TYPE:
//    if (GET_OPTIONAL_PROGRAM_STOP() == ON) {
//        emcTrajPause();
//        if (emcStatus->task.interpState != EMC_TASK_INTERP::PAUSED) {
//        interpResumeState = emcStatus->task.interpState;
//        }
//        emcStatus->task.interpState = EMC_TASK_INTERP::PAUSED;
//        emcStatus->task.task_paused = 1;
//    }
//    retval = 0;
    break;

    case EMC_TASK_PLAN_REVERSE_TYPE:
//    emcTrajReverse();
//    retval = 0;
    break;

    case EMC_TASK_PLAN_FORWARD_TYPE:
//    emcTrajForward();
//    retval = 0;
    break;

    case EMC_TASK_PLAN_RESUME_TYPE:
//    emcTrajResume();
//    emcStatus->task.interpState = interpResumeState;
//    emcStatus->task.task_paused = 0;
//    stepping = 0;
//    steppingWait = 0;
//    retval = 0;
    break;

    case EMC_TASK_PLAN_END_TYPE:
    retval = 0;
    break;

    case EMC_TASK_PLAN_INIT_TYPE:
//    retval = emcTaskPlanInit();
//    if (retval > INTERP_MIN_ERROR) {
//        retval = -1;
//    }
    break;

    case EMC_TASK_PLAN_SYNCH_TYPE:
//    retval = emcTaskPlanSynch();
//    if (retval > INTERP_MIN_ERROR) {
//        retval = -1;
//    }
    break;

    case EMC_TASK_PLAN_SET_OPTIONAL_STOP_TYPE:
//    os_msg = (EMC_TASK_PLAN_SET_OPTIONAL_STOP *) cmd;
//    emcTaskPlanSetOptionalStop(os_msg->state);
//    retval = 0;
    break;

    case EMC_TASK_PLAN_SET_BLOCK_DELETE_TYPE:
//    bd_msg = (EMC_TASK_PLAN_SET_BLOCK_DELETE *) cmd;
//    emcTaskPlanSetBlockDelete(bd_msg->state);
//    retval = 0;
    break;

     default:
    // unrecognized command
    retval = 0;		// don't consider this an error
    break;
    }

    return retval;
}

EMCTask::~EMCTask() {}
