#include "emcParas.h"
#include "emc.hh"
#include "emcTask.h"
#include "rs274ngc_interp.hh"	// the interpreter
#include "emcIniFile.hh"
#include "emcglb.h"
#include "interpl.hh"
#include "motionTask.h"
#include "homing.h"
#include "axis.h"
#include "tp.h"
#include "motion.h"
#include "mot_priv.h"


#define ALL_JOINTS emcmotConfig->numJoints
// number of kinematics-only joints:
#define NO_OF_KINS_JOINTS (ALL_JOINTS - emcmotConfig->numExtraJoints)
#define IS_EXTRA_JOINT(jno) (jno >= NO_OF_KINS_JOINTS)
// 0-based Joint numbering:
// kinematic-only jno.s: [0                 ... (NO_OF_KINS_JOINTS -1) ]
// extrajoint     jno.s: [NO_OF_KINS_JOINTS ... (ALL_JOINTS  -1) ]

 /* rtapi_get_time() returns a nanosecond value. In time, we should use a u64
    value for all calcs and only do the conversion to seconds when it is
    really needed. */
#define etime() (((double) rtapi_get_time()) / 1.0e9)

/* macros for reading, writing bit flags */

/* motion flags */

#define GET_MOTION_ERROR_FLAG() (emcmotStatus->motionFlag & EMCMOT_MOTION_ERROR_BIT ? 1 : 0)

#define SET_MOTION_ERROR_FLAG(fl) if (fl) emcmotStatus->motionFlag |= EMCMOT_MOTION_ERROR_BIT; else emcmotStatus->motionFlag &= ~EMCMOT_MOTION_ERROR_BIT;

#define GET_MOTION_COORD_FLAG() (emcmotStatus->motionFlag & EMCMOT_MOTION_COORD_BIT ? 1 : 0)

#define SET_MOTION_COORD_FLAG(fl) if (fl) emcmotStatus->motionFlag |= EMCMOT_MOTION_COORD_BIT; else emcmotStatus->motionFlag &= ~EMCMOT_MOTION_COORD_BIT;

#define GET_MOTION_TELEOP_FLAG() (emcmotStatus->motionFlag & EMCMOT_MOTION_TELEOP_BIT ? 1 : 0)

#define SET_MOTION_TELEOP_FLAG(fl) if (fl) emcmotStatus->motionFlag |= EMCMOT_MOTION_TELEOP_BIT; else emcmotStatus->motionFlag &= ~EMCMOT_MOTION_TELEOP_BIT;

#define GET_MOTION_INPOS_FLAG() (emcmotStatus->motionFlag & EMCMOT_MOTION_INPOS_BIT ? 1 : 0)

#define SET_MOTION_INPOS_FLAG(fl) if (fl) emcmotStatus->motionFlag |= EMCMOT_MOTION_INPOS_BIT; else emcmotStatus->motionFlag &= ~EMCMOT_MOTION_INPOS_BIT;

#define GET_MOTION_ENABLE_FLAG() (emcmotStatus->motionFlag & EMCMOT_MOTION_ENABLE_BIT ? 1 : 0)

#define SET_MOTION_ENABLE_FLAG(fl) if (fl) emcmotStatus->motionFlag |= EMCMOT_MOTION_ENABLE_BIT; else emcmotStatus->motionFlag &= ~EMCMOT_MOTION_ENABLE_BIT;

/* joint flags */

#define GET_JOINT_ENABLE_FLAG(joint) ((joint)->flag & EMCMOT_JOINT_ENABLE_BIT ? 1 : 0)

#define SET_JOINT_ENABLE_FLAG(joint,fl) if (fl) (joint)->flag |= EMCMOT_JOINT_ENABLE_BIT; else (joint)->flag &= ~EMCMOT_JOINT_ENABLE_BIT;

#define SET_JOINT_ACTIVE_FLAG(joint,fl) if (fl) (joint)->flag |= EMCMOT_JOINT_ACTIVE_BIT; else (joint)->flag &= ~EMCMOT_JOINT_ACTIVE_BIT;

#define SET_JOINT_INPOS_FLAG(joint,fl) if (fl) (joint)->flag |= EMCMOT_JOINT_INPOS_BIT; else (joint)->flag &= ~EMCMOT_JOINT_INPOS_BIT;

#define GET_JOINT_ERROR_FLAG(joint) ((joint)->flag & EMCMOT_JOINT_ERROR_BIT ? 1 : 0)

#define SET_JOINT_ERROR_FLAG(joint,fl) if (fl) (joint)->flag |= EMCMOT_JOINT_ERROR_BIT; else (joint)->flag &= ~EMCMOT_JOINT_ERROR_BIT;

#define GET_JOINT_PHL_FLAG(joint) ((joint)->flag & EMCMOT_JOINT_MAX_HARD_LIMIT_BIT ? 1 : 0)

#define SET_JOINT_PHL_FLAG(joint,fl) if (fl) (joint)->flag |= EMCMOT_JOINT_MAX_HARD_LIMIT_BIT; else (joint)->flag &= ~EMCMOT_JOINT_MAX_HARD_LIMIT_BIT;

#define GET_JOINT_NHL_FLAG(joint) ((joint)->flag & EMCMOT_JOINT_MIN_HARD_LIMIT_BIT ? 1 : 0)

#define SET_JOINT_NHL_FLAG(joint,fl) if (fl) (joint)->flag |= EMCMOT_JOINT_MIN_HARD_LIMIT_BIT; else (joint)->flag &= ~EMCMOT_JOINT_MIN_HARD_LIMIT_BIT;


#define GET_JOINT_FERROR_FLAG(joint) ((joint)->flag & EMCMOT_JOINT_FERROR_BIT ? 1 : 0)

#define SET_JOINT_FERROR_FLAG(joint,fl) if (fl) (joint)->flag |= EMCMOT_JOINT_FERROR_BIT; else (joint)->flag &= ~EMCMOT_JOINT_FERROR_BIT;

#define GET_JOINT_FAULT_FLAG(joint) ((joint)->flag & EMCMOT_JOINT_FAULT_BIT ? 1 : 0)

#define SET_JOINT_FAULT_FLAG(joint,fl) if (fl) (joint)->flag |= EMCMOT_JOINT_FAULT_BIT; else (joint)->flag &= ~EMCMOT_JOINT_FAULT_BIT;


emcmot_command_t MotionTask::emcmotCommandEntry;
emcmot_status_t MotionTask::emcmotStatusEntry;
emcmot_config_t MotionTask::emcmotConfigEntry;
emcmot_internal_t MotionTask::emcmotInternalEntry;
emcmot_error_t MotionTask::emcmotErrorEntry;

emcmot_command_t *MotionTask::emcmotCommand = &MotionTask::emcmotCommandEntry;
emcmot_status_t *MotionTask::emcmotStatus = &MotionTask::emcmotStatusEntry;
emcmot_config_t *MotionTask::emcmotConfig = &MotionTask::emcmotConfigEntry;
emcmot_internal_t *MotionTask::emcmotInternal = &MotionTask::emcmotInternalEntry;
emcmot_error_t *MotionTask::emcmotError = &MotionTask::emcmotErrorEntry;	/* unused for RT_FIFO */
emcmot_joint_t MotionTask::joints[EMCMOT_MAX_JOINTS];


//extern int motion_num_spindles;

//void MotionPlanner::CmdHandler()
//{
//    int joint_num, spindle_num;
//    int n,s0,s1;
//    emcmot_joint_t *joint;
//    double tmp1;
//    emcmot_comp_entry_t *comp_entry;
//    char issue_atspeed = 0;
//    int abort = 0;
//    char* emsg = "";

//    if (emcmotCommand->commandNum != emcmotStatus->commandNumEcho) {
//        /* increment head count-- we'll be modifying emcmotStatus */
//        emcmotStatus->head++;
//        emcmotInternal->head++;

//        /* got a new command-- echo command and number... */
//        emcmotStatus->commandEcho = emcmotCommand->command;
//        emcmotStatus->commandNumEcho = emcmotCommand->commandNum;

//        /* clear status value by default */
//        emcmotStatus->commandStatus = EMCMOT_COMMAND_OK;

//        /* ...and process command */

//        joint = 0;
//        joint_num = emcmotCommand->joint;

//        //-----------------------------------------------------------------------------
//        // joints_axes test for unexpected conditions
//        // example: non-cooperating guis
//        // example: attempt to jog locking indexer axis letter
//        if (   emcmotCommand->command == EMCMOT_JOG_CONT
//            || emcmotCommand->command == EMCMOT_JOG_INCR
//            || emcmotCommand->command == EMCMOT_JOG_ABS
//           ) {
//           if (GET_MOTION_TELEOP_FLAG() && (emcmotCommand->axis < 0)) {
//               emsg = "command.com teleop: unexpected negative axis_num";
//               if (joint_num >= 0) {
//                   emsg = "Mode is TELEOP, cannot jog joint";
//               }
//               abort = 1;
//           }
//           if (!GET_MOTION_TELEOP_FLAG() && joint_num < 0) {
//               emsg = "command.com !teleop: unexpected negative joint_num";
//               if (emcmotCommand->axis >= 0) {
//                   emsg = "Mode is NOT TELEOP, cannot jog axis coordinate";
//               }
//               abort = 1;
//           }
//           if (   !GET_MOTION_TELEOP_FLAG()
//               && (joint_num >= ALL_JOINTS || joint_num <  0)
//              ) {
//               rtapi_print_msg(RTAPI_MSG_ERR,
//                    "Joint jog requested for undefined joint number=%d (min=0,max=%d)",
//                    joint_num,ALL_JOINTS-1);
//               return;
//           }
//           if (GET_MOTION_TELEOP_FLAG()) {
//                if ( (emcmotCommand->axis >= 0) && (axis_get_locking_joint(emcmotCommand->axis) >= 0) ) {
//                    rtapi_print_msg(RTAPI_MSG_ERR,
//                    "Cannot jog a locking indexer AXIS_%c,joint_num=%d\n",
//                    "XYZABCUVW"[emcmotCommand->axis], axis_get_locking_joint(emcmotCommand->axis));
//                    return;
//                }
//           }
//        }
//        if (abort) {
//          switch (emcmotCommand->command) {
//          case EMCMOT_JOG_CONT:
//               rtapi_print_msg(RTAPI_MSG_ERR,"JOG_CONT %s\n",emsg);
//               break;
//          case EMCMOT_JOG_INCR:
//               rtapi_print_msg(RTAPI_MSG_ERR,"JOG_INCR %s\n",emsg);
//               break;
//          case EMCMOT_JOG_ABS:
//               rtapi_print_msg(RTAPI_MSG_ERR,"JOG_ABS %s\n",emsg);
//               break;
//          default: break;
//          }
//          return;
//        }

//        if (joint_num >= 0 && joint_num < ALL_JOINTS) {
//            joint = &joints[joint_num];
//            if (   (   emcmotCommand->command == EMCMOT_JOG_CONT
//                    || emcmotCommand->command == EMCMOT_JOG_INCR
//                    || emcmotCommand->command == EMCMOT_JOG_ABS
//                   )
//                && !(GET_MOTION_TELEOP_FLAG())
//                && get_home_is_synchronized(joint_num)
//                && !get_homing_is_active()
//               ) {
//                  if (emcmotConfig->kinType == KINEMATICS_IDENTITY) {
//                      rtapi_print_msg(RTAPI_MSG_ERR,
//                      "Homing is REQUIRED to jog requested coordinate\n"
//                      "because joint (%d) home_sequence is synchronized (%d)\n"
//                      ,joint_num,get_home_sequence(joint_num));
//                  } else {
//                      rtapi_print_msg(RTAPI_MSG_ERR,
//                      "Cannot jog joint %d because home_sequence is synchronized (%d)\n"
//                      ,joint_num,get_home_sequence(joint_num));
//                  }
//                  return;
//            }
//        }
//    switch (emcmotCommand->command) {
//    case EMCMOT_ABORT:
//        /* abort motion */
//        /* can happen at any time */
//        /* this command attempts to stop all machine motion. it looks at
//           the current mode and acts accordingly, if in teleop mode, it
//           sets the desired velocities to zero, if in coordinated mode,
//           it calls the traj planner abort function (don't know what that
//           does yet), and if in free mode, it disables the free mode traj
//           planners which stops joint motion */
//        rtapi_print_msg(RTAPI_MSG_DBG, "ABORT");
//        rtapi_print_msg(RTAPI_MSG_DBG, " %d", joint_num);
//        /* check for coord or free space motion active */
//        if (GET_MOTION_TELEOP_FLAG()) {
//                axis_jog_abort_all(0);
//        } else if (GET_MOTION_COORD_FLAG()) {
//        tpAbort(&emcmotInternal->coord_tp);
//        } else {
//        for (joint_num = 0; joint_num < ALL_JOINTS; joint_num++) {
//            /* point to joint struct */
//            joint = &joints[joint_num];
//            /* tell joint planner to stop */
//            joint->free_tp.enable = 0;
//            /* stop homing if in progress */
//            if ( ! get_home_is_idle(joint_num)) {
//            do_cancel_homing(joint_num);
//            }
//        }
//        }
//            SET_MOTION_ERROR_FLAG(0);
//        /* clear joint errors (regardless of mode) */
//        for (joint_num = 0; joint_num < ALL_JOINTS; joint_num++) {
//        /* point to joint struct */
//        joint = &joints[joint_num];
//        /* update status flags */
//        SET_JOINT_ERROR_FLAG(joint, 0);
//        SET_JOINT_FAULT_FLAG(joint, 0);
//        }
//        emcmotStatus->paused = 0;
//        break;

//    case EMCMOT_JOG_ABORT:
//        /* abort one joint number or axis number */
//        /* can happen at any time */
//        if (GET_MOTION_TELEOP_FLAG()) {
//            /* tell teleop planner to stop */
//            if ((emcmotCommand->axis >= 0) && (emcmotCommand->axis < EMCMOT_MAX_AXIS)) {
//                axis_jog_abort(emcmotCommand->axis, 0);
//            }
//        } else {
//            if (joint == 0) { break; }
//            /* tell joint planner to stop */
//            joint->free_tp.enable    = 0;
//            joint->kb_jjog_active    = 0;
//            joint->wheel_jjog_active = 0;
//            /* stop homing if in progress */
//            if ( !get_home_is_idle(joint_num) ) {
//                do_cancel_homing(joint_num);
//            }
//            /* update status flags */
//            SET_JOINT_ERROR_FLAG(joint, 0);
//        }
//        break;

//    case EMCMOT_FREE:
//            axis_jog_abort_all(0);
//        /* change the mode to free mode motion (joint mode) */
//        /* can be done at any time */
//        /* this code doesn't actually make the transition, it merely
//           requests the transition by clearing a couple of flags */
//        /* reset the emcmotInternal->coordinating flag to defer transition
//           to controller cycle */
//        rtapi_print_msg(RTAPI_MSG_DBG, "FREE");
//        emcmotInternal->coordinating = 0;
//        emcmotInternal->teleoperating = 0;
//        break;

//    case EMCMOT_COORD:
//        /* change the mode to coordinated axis motion */
//        /* can be done at any time */
//        /* this code doesn't actually make the transition, it merely
//           tests a condition and then sets a flag requesting the
//           transition */
//        /* set the emcmotInternal->coordinating flag to defer transition to
//           controller cycle */

//        rtapi_print_msg(RTAPI_MSG_DBG, "COORD");
//        emcmotInternal->coordinating = 1;
//        emcmotInternal->teleoperating = 0;
//        if (emcmotConfig->kinType != KINEMATICS_IDENTITY) {
//        if (!get_allhomed()) {
//            reportError
//            (_("all joints must be homed before going into coordinated mode"));
//            emcmotInternal->coordinating = 0;
//            break;
//        }
//        }
//        break;

//    case EMCMOT_TELEOP:
//        rtapi_print_msg(RTAPI_MSG_DBG, "TELEOP");
//            switch_to_teleop_mode();
//        break;

//    case EMCMOT_SET_NUM_JOINTS:
//        /* set the global NUM_JOINTS, which must be between 1 and
//           EMCMOT_MAX_JOINTS, inclusive.
//           Called  by task using [KINS]JOINTS= which is typically
//           the same value as the motmod num_joints= parameter
//        */
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_NUM_JOINTS");
//        rtapi_print_msg(RTAPI_MSG_DBG, " %d", emcmotCommand->joint);
//        if (( emcmotCommand->joint <= 0 ) ||
//        ( emcmotCommand->joint > EMCMOT_MAX_JOINTS )) {
//        break;
//        }
//        ALL_JOINTS = emcmotCommand->joint;
//        break;

//    case EMCMOT_SET_NUM_SPINDLES:
//        /* set the global NUM_SPINDLES, which must be between 1 and
//           EMCMOT_MAX_SPINDLES, inclusive and less than or equal to
//           the number of spindles configured for the motion module
//           (motion_num_spindles)
//        */
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_NUM_SPINDLES");
//        rtapi_print_msg(RTAPI_MSG_DBG, " %d", emcmotCommand->spindle);
//        if (   emcmotCommand->spindle > motion_num_spindles
//            || emcmotCommand->spindle <= 0
//            || emcmotCommand->spindle > EMCMOT_MAX_SPINDLES
//           ) {
//            reportError("Problem:\n"
//                        "  motmod configured for %d spindles\n"
//                        "  but command requests %d spindles\n"
//                        "  Using: %d spindles",
//                        motion_num_spindles,
//                        emcmotCommand->spindle,
//                        motion_num_spindles
//                       );
//            emcmotConfig->numSpindles = motion_num_spindles;
//        } else {
//            emcmotConfig->numSpindles = emcmotCommand->spindle;
//        }
//        break;

//    case EMCMOT_SET_WORLD_HOME:
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_WORLD_HOME");
//        emcmotStatus->world_home = emcmotCommand->pos;
//        break;

//    case EMCMOT_SET_JOINT_HOMING_PARAMS:
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_JOINT_HOMING_PARAMS");
//        rtapi_print_msg(RTAPI_MSG_DBG, " %d", joint_num);
//        emcmot_config_change();
//        if (joint == 0) {
//        break;
//        }
//        set_joint_homing_params(joint_num,
//                                emcmotCommand->offset,
//                                emcmotCommand->home,
//                                emcmotCommand->home_final_vel,
//                                emcmotCommand->search_vel,
//                                emcmotCommand->latch_vel,
//                                emcmotCommand->flags,
//                                emcmotCommand->home_sequence,
//                                emcmotCommand->volatile_home
//                               );
//        break;

//    case EMCMOT_UPDATE_JOINT_HOMING_PARAMS:
//        rtapi_print_msg(RTAPI_MSG_DBG, "UPDATE_JOINT_HOMING_PARAMS");
//        rtapi_print_msg(RTAPI_MSG_DBG, " %d", joint_num);
//        emcmot_config_change();
//        if (joint == 0) {
//        break;
//        }
//        update_joint_homing_params(joint_num,
//                                   emcmotCommand->offset,
//                                   emcmotCommand->home,
//                                   emcmotCommand->home_sequence
//                                   );
//        break;

//    case EMCMOT_OVERRIDE_LIMITS:
//        /* this command can be issued with joint < 0 to re-enable
//           limits, but they are automatically re-enabled at the
//           end of the next jog */
//        rtapi_print_msg(RTAPI_MSG_DBG, "OVERRIDE_LIMITS");
//        rtapi_print_msg(RTAPI_MSG_DBG, " %d", joint_num);
//        if (joint_num < 0) {
//        /* don't override limits */
//        rtapi_print_msg(RTAPI_MSG_DBG, "override off");
//        emcmotStatus->overrideLimitMask = 0;
//        } else {
//        rtapi_print_msg(RTAPI_MSG_DBG, "override on");
//        emcmotStatus->overrideLimitMask = 0;
//        for (joint_num = 0; joint_num < ALL_JOINTS; joint_num++) {
//            /* point at joint data */
//            joint = &joints[joint_num];
//            /* only override limits that are currently tripped */
//            if ( GET_JOINT_NHL_FLAG(joint) ) {
//            emcmotStatus->overrideLimitMask |= (1 << (joint_num*2));
//            }
//            if ( GET_JOINT_PHL_FLAG(joint) ) {
//            emcmotStatus->overrideLimitMask |= (2 << (joint_num*2));
//            }
//        }
//        }
//        emcmotInternal->overriding = 0;
//        for (joint_num = 0; joint_num < ALL_JOINTS; joint_num++) {
//        /* point at joint data */
//        joint = &joints[joint_num];
//        /* clear joint errors */
//        SET_JOINT_ERROR_FLAG(joint, 0);
//        }
//        break;

//    case EMCMOT_SET_JOINT_MOTOR_OFFSET:
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_JOINT_MOTOR_OFFSET");
//        rtapi_print_msg(RTAPI_MSG_DBG, " %d", joint_num);
//        if(joint == 0) {
//        break;
//        }
//        joint->motor_offset = emcmotCommand->motor_offset;
//        break;

//    case EMCMOT_SET_JOINT_POSITION_LIMITS:
//        /* set the position limits for the joint */
//        /* can be done at any time */
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_JOINT_POSITION_LIMITS");
//        rtapi_print_msg(RTAPI_MSG_DBG, " %d", joint_num);
//        emcmot_config_change();
//        if (joint == 0) {
//        break;
//        }
//        joint->min_pos_limit = emcmotCommand->minLimit;
//        joint->max_pos_limit = emcmotCommand->maxLimit;
//        break;

//    case EMCMOT_SET_JOINT_BACKLASH:
//        /* set the backlash for the joint */
//        /* can be done at any time */
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_JOINT_BACKLASH");
//        rtapi_print_msg(RTAPI_MSG_DBG, " %d", joint_num);
//        emcmot_config_change();
//        if (joint == 0) {
//        break;
//        }
//        joint->backlash = emcmotCommand->backlash;
//        break;

//        /*
//           Max and min ferror work like this: limiting ferror is
//           determined by slope of ferror line, = maxFerror/limitVel ->
//           limiting ferror = maxFerror/limitVel * vel. If ferror <
//           minFerror then OK else if ferror < limiting ferror then OK
//           else ERROR */
//    case EMCMOT_SET_JOINT_MAX_FERROR:
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_JOINT_MAX_FERROR");
//        rtapi_print_msg(RTAPI_MSG_DBG, " %d", joint_num);
//        emcmot_config_change();
//        if (joint == 0 || emcmotCommand->maxFerror < 0.0) {
//        break;
//        }
//        joint->max_ferror = emcmotCommand->maxFerror;
//        break;

//    case EMCMOT_SET_JOINT_MIN_FERROR:
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_JOINT_MIN_FERROR");
//        rtapi_print_msg(RTAPI_MSG_DBG, " %d", joint_num);
//        emcmot_config_change();
//        if (joint == 0 || emcmotCommand->minFerror < 0.0) {
//        break;
//        }
//        joint->min_ferror = emcmotCommand->minFerror;
//        break;

//    case EMCMOT_JOG_CONT:
//        /* do a continuous jog, implemented as an incremental jog to the
//           limit.  When the user lets go of the button an abort will
//           stop the jog. */
//        rtapi_print_msg(RTAPI_MSG_DBG, "JOG_CONT");
//        rtapi_print_msg(RTAPI_MSG_DBG, " %d", joint_num);
//        if (!GET_MOTION_ENABLE_FLAG()) {
//        reportError(_("Can't jog joint when not enabled."));
//        SET_JOINT_ERROR_FLAG(joint, 1);
//        break;
//        }
//            // cannot jog if jog-inhibit is TRUE
//            if (*(emcmot_hal_data->jog_inhibit)){
//                    reportError(_("Cannot jog while jog-inhibit is active."));
//                break;
//            }
//        if ( get_homing_is_active() ) {
//        reportError(_("Can't jog any joints while homing."));
//        SET_JOINT_ERROR_FLAG(joint, 1);
//        break;
//        }
//            if (!GET_MOTION_TELEOP_FLAG()) {
//            if (joint->wheel_jjog_active) {
//            /* can't do two kinds of jog at once */
//            break;
//            }
//                if (get_home_needs_unlock_first(joint_num) ) {
//                    reportError("Can't jog locking joint_num=%d",joint_num);
//                    SET_JOINT_ERROR_FLAG(joint, 1);
//                    break;
//                }
//            /* don't jog further onto limits */
//            if (!joint_jog_ok(joint_num, emcmotCommand->vel)) {
//            SET_JOINT_ERROR_FLAG(joint, 1);
//            break;
//            }
//            /* set destination of jog */
//            refresh_jog_limits(joint,joint_num);
//            if (emcmotCommand->vel > 0.0) {
//            joint->free_tp.pos_cmd = joint->max_jog_limit;
//            } else {
//            joint->free_tp.pos_cmd = joint->min_jog_limit;
//            }
//            /* set velocity of jog */
//            joint->free_tp.max_vel = fabs(emcmotCommand->vel);
//            /* use max joint accel */
//            joint->free_tp.max_acc = joint->acc_limit;
//            /* lock out other jog sources */
//            joint->kb_jjog_active = 1;
//            /* and let it go */
//            joint->free_tp.enable = 1;
//                axis_jog_abort_all(0);
//            /*! \todo FIXME - should we really be clearing errors here? */
//            SET_JOINT_ERROR_FLAG(joint, 0);
//            /* clear joints homed flag(s) if we don't have forward kins.
//               Otherwise, a transition into coordinated mode will incorrectly
//               assume the homed position. Do all if they've all been moved
//               since homing, otherwise just do this one */
//            clearHomes(joint_num);
//            } else {
//                // TELEOP  JOG_CONT
//                if (GET_MOTION_ERROR_FLAG()) { break; }
//                for (joint_num = 0; joint_num < ALL_JOINTS; joint_num++) {
//                    joint = &joints[joint_num];
//                    if (joint != 0) { joint->free_tp.enable = 0; }
//                }
//                axis_jog_cont(emcmotCommand->axis, emcmotCommand->vel, servo_period);
//            }
//        break;

//    case EMCMOT_JOG_INCR:
//        /* do an incremental jog */
//        rtapi_print_msg(RTAPI_MSG_DBG, "JOG_INCR");
//        rtapi_print_msg(RTAPI_MSG_DBG, " %d", joint_num);
//        if (!GET_MOTION_ENABLE_FLAG()) {
//        reportError(_("Can't jog joint when not enabled."));
//        SET_JOINT_ERROR_FLAG(joint, 1);
//        break;
//        }
//            // cannot jog if jog-inhibit is TRUE
//            if (*(emcmot_hal_data->jog_inhibit)){
//                    reportError(_("Cannot jog while jog-inhibit is active."));
//                break;
//            }
//        if ( get_homing_is_active() ) {
//        reportError(_("Can't jog any joint while homing."));
//        SET_JOINT_ERROR_FLAG(joint, 1);
//        break;
//        }
//            if (!GET_MOTION_TELEOP_FLAG()) {
//            if (joint->wheel_jjog_active) {
//            /* can't do two kinds of jog at once */
//            break;
//            }
//                if (get_home_needs_unlock_first(joint_num) ) {
//                    reportError("Can't jog locking joint_num=%d",joint_num);
//                    SET_JOINT_ERROR_FLAG(joint, 1);
//                    break;
//                }
//            /* don't jog further onto limits */
//            if (!joint_jog_ok(joint_num, emcmotCommand->vel)) {
//            SET_JOINT_ERROR_FLAG(joint, 1);
//            break;
//            }
//            /* set target position for jog */
//            if (emcmotCommand->vel > 0.0) {
//            tmp1 = joint->free_tp.pos_cmd + emcmotCommand->offset;
//            } else {
//            tmp1 = joint->free_tp.pos_cmd - emcmotCommand->offset;
//            }
//            /* don't jog past limits */
//            refresh_jog_limits(joint,joint_num);
//            if (tmp1 > joint->max_jog_limit) {
//            break;
//            }
//            if (tmp1 < joint->min_jog_limit) {
//            break;
//            }
//            /* set target position */
//            joint->free_tp.pos_cmd = tmp1;
//            /* set velocity of jog */
//            joint->free_tp.max_vel = fabs(emcmotCommand->vel);
//            /* use max joint accel */
//            joint->free_tp.max_acc = joint->acc_limit;
//            /* lock out other jog sources */
//            joint->kb_jjog_active = 1;
//            /* and let it go */
//            joint->free_tp.enable = 1;
//                axis_jog_abort_all(0);
//            SET_JOINT_ERROR_FLAG(joint, 0);
//            /* clear joint homed flag(s) if we don't have forward kins.
//               Otherwise, a transition into coordinated mode will incorrectly
//               assume the homed position. Do all if they've all been moved
//               since homing, otherwise just do this one */
//            clearHomes(joint_num);
//            } else {
//                // TELEOP JOG_INCR
//                if (GET_MOTION_ERROR_FLAG()) { break; }
//                axis_jog_incr(emcmotCommand->axis, emcmotCommand->offset, emcmotCommand->vel, servo_period);
//                for (joint_num = 0; joint_num < ALL_JOINTS; joint_num++) {
//                    joint = &joints[joint_num];
//                    if (joint != 0) { joint->free_tp.enable = 0; }
//                }
//            }
//        break;

//    case EMCMOT_JOG_ABS:
//        /* do an absolute jog */
//        rtapi_print_msg(RTAPI_MSG_DBG, "JOG_ABS");
//        rtapi_print_msg(RTAPI_MSG_DBG, " %d", joint_num);
//        if (joint == 0) {
//        break;
//        }
//        if (!GET_MOTION_ENABLE_FLAG()) {
//        reportError(_("Can't jog joint when not enabled."));
//        SET_JOINT_ERROR_FLAG(joint, 1);
//        break;
//        }
//            // cannot jog if jog-inhibit is TRUE
//            if (*(emcmot_hal_data->jog_inhibit)){
//                    reportError(_("Cannot jog while jog-inhibit is active."));
//                break;
//            }
//        if ( get_homing_is_active() ) {
//        reportError(_("Can't jog any joints while homing."));
//        SET_JOINT_ERROR_FLAG(joint, 1);
//        break;
//        }
//            if (!GET_MOTION_TELEOP_FLAG()) {
//                // FREE JOG_ABS
//                if (joint->wheel_jjog_active) {
//                    /* can't do two kinds of jog at once */
//                    break;
//                }
//                /* don't jog further onto limits */
//                if (!joint_jog_ok(joint_num, emcmotCommand->vel)) {
//                    SET_JOINT_ERROR_FLAG(joint, 1);
//                    break;
//                }
//                /*! \todo FIXME-- use 'goal' instead */
//                joint->free_tp.pos_cmd = emcmotCommand->offset;
//                /* don't jog past limits */
//                refresh_jog_limits(joint,joint_num);
//                if (joint->free_tp.pos_cmd > joint->max_jog_limit) {
//                    joint->free_tp.pos_cmd = joint->max_jog_limit;
//                }
//                if (joint->free_tp.pos_cmd < joint->min_jog_limit) {
//                    joint->free_tp.pos_cmd = joint->min_jog_limit;
//                }
//                /* set velocity of jog */
//                joint->free_tp.max_vel = fabs(emcmotCommand->vel);
//                /* use max joint accel */
//                joint->free_tp.max_acc = joint->acc_limit;
//                /* lock out other jog sources */
//                joint->kb_jjog_active = 1;
//                /* and let it go */
//                joint->free_tp.enable = 1;
//                SET_JOINT_ERROR_FLAG(joint, 0);
//                /* clear joint homed flag(s) if we don't have forward kins.
//                   Otherwise, a transition into coordinated mode will incorrectly
//                   assume the homed position. Do all if they've all been moved
//                   since homing, otherwise just do this one */
//                clearHomes(joint_num);
//            } else {
//                // TELEOP JOG_ABS
//                axis_jog_abs(emcmotCommand->axis, emcmotCommand->offset, emcmotCommand->vel);
//                for (joint_num = 0; joint_num < ALL_JOINTS; joint_num++) {
//                   joint = &joints[joint_num];
//                   if (joint != 0) { joint->free_tp.enable = 0; }
//                }
//                return;
//            }
//            break;

//    case EMCMOT_SET_TERM_COND:
//        /* sets termination condition for motion emcmotInternal->coord_tp */
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_TERM_COND");
//        tpSetTermCond(&emcmotInternal->coord_tp, emcmotCommand->termCond, emcmotCommand->tolerance);
//        break;

//    case EMCMOT_SET_SPINDLESYNC:
//        tpSetSpindleSync(&emcmotInternal->coord_tp, emcmotCommand->spindle, emcmotCommand->spindlesync, emcmotCommand->flags);
//        break;

//    case EMCMOT_SET_LINE:
//        /* emcmotInternal->coord_tp up a linear move */
//        /* requires motion enabled, coordinated mode, not on limits */
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_LINE");
//        if (!GET_MOTION_COORD_FLAG() || !GET_MOTION_ENABLE_FLAG()) {
//        reportError(_("need to be enabled, in coord mode for linear move"));
//        emcmotStatus->commandStatus = EMCMOT_COMMAND_INVALID_COMMAND;
//        SET_MOTION_ERROR_FLAG(1);
//        break;
//        } else if (!inRange(emcmotCommand->pos, emcmotCommand->id, "Linear")) {
//        reportError(_("invalid params in linear command"));
//        emcmotStatus->commandStatus = EMCMOT_COMMAND_INVALID_PARAMS;
//        tpAbort(&emcmotInternal->coord_tp);
//        SET_MOTION_ERROR_FLAG(1);
//        break;
//        } else if (!limits_ok()) {
//        reportError(_("can't do linear move with limits exceeded"));
//        emcmotStatus->commandStatus = EMCMOT_COMMAND_INVALID_PARAMS;
//        tpAbort(&emcmotInternal->coord_tp);
//        SET_MOTION_ERROR_FLAG(1);
//        break;
//        }

//        if(emcmotStatus->atspeed_next_feed && is_feed_type(emcmotCommand->motion_type) ) {
//            issue_atspeed = 1;
//            emcmotStatus->atspeed_next_feed = 0;
//        }
//        if(!is_feed_type(emcmotCommand->motion_type) &&
//                emcmotStatus->spindle_status[emcmotCommand->spindle].css_factor) {
//            emcmotStatus->atspeed_next_feed = 1;
//        }

//        /* append it to the emcmotInternal->coord_tp */
//        tpSetId(&emcmotInternal->coord_tp, emcmotCommand->id);
//        int res_addline = tpAddLine(&emcmotInternal->coord_tp,
//                    emcmotCommand->pos,
//                    emcmotCommand->motion_type,
//                    emcmotCommand->vel,
//                    emcmotCommand->ini_maxvel,
//                    emcmotCommand->acc,
//                    emcmotStatus->enables_new,
//                    issue_atspeed,
//                    emcmotCommand->turn,
//                    emcmotCommand->tag);
//        //KLUDGE ignore zero length line
//        if (res_addline < 0) {
//            reportError(_("can't add linear move at line %d, error code %d"),
//                    emcmotCommand->id, res_addline);
//            emcmotStatus->commandStatus = EMCMOT_COMMAND_BAD_EXEC;
//            tpAbort(&emcmotInternal->coord_tp);
//            SET_MOTION_ERROR_FLAG(1);
//            break;
//        } else if (res_addline != 0) {
//            //TODO make this hand-shake more explicit
//            //KLUDGE Non fatal error, need to restore state so that the next
//            //line properly handles at_speed
//            if (issue_atspeed) {
//                emcmotStatus->atspeed_next_feed = 1;
//            }
//        } else {
//        SET_MOTION_ERROR_FLAG(0);
//        /* set flag that indicates all joints need rehoming, if any
//           joint is moved in joint mode, for machines with no forward
//           kins */
//        rehomeAll = 1;
//        }
//        break;

//    case EMCMOT_SET_CIRCLE:
//        /* emcmotInternal->coord_tp up a circular move */
//        /* requires coordinated mode, enable on, not on limits */
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_CIRCLE");
//        if (!GET_MOTION_COORD_FLAG() || !GET_MOTION_ENABLE_FLAG()) {
//        reportError(_("need to be enabled, in coord mode for circular move"));
//        emcmotStatus->commandStatus = EMCMOT_COMMAND_INVALID_COMMAND;
//        SET_MOTION_ERROR_FLAG(1);
//        break;
//        } else if (!inRange(emcmotCommand->pos, emcmotCommand->id, "Circular")) {
//        emcmotStatus->commandStatus = EMCMOT_COMMAND_INVALID_PARAMS;
//        tpAbort(&emcmotInternal->coord_tp);
//        SET_MOTION_ERROR_FLAG(1);
//        break;
//        } else if (!limits_ok()) {
//        reportError(_("can't do circular move with limits exceeded"));
//        emcmotStatus->commandStatus = EMCMOT_COMMAND_INVALID_PARAMS;
//        tpAbort(&emcmotInternal->coord_tp);
//        SET_MOTION_ERROR_FLAG(1);
//        break;
//        }
//            if(emcmotStatus->atspeed_next_feed) {
//                issue_atspeed = 1;
//                emcmotStatus->atspeed_next_feed = 0;
//            }
//        /* append it to the emcmotInternal->coord_tp */
//        tpSetId(&emcmotInternal->coord_tp, emcmotCommand->id);
//        int res_addcircle = tpAddCircle(&emcmotInternal->coord_tp, emcmotCommand->pos,
//                            emcmotCommand->center, emcmotCommand->normal,
//                            emcmotCommand->turn, emcmotCommand->motion_type,
//                            emcmotCommand->vel, emcmotCommand->ini_maxvel,
//                            emcmotCommand->acc, emcmotStatus->enables_new,
//                issue_atspeed, emcmotCommand->tag);
//        if (res_addcircle < 0) {
//            reportError(_("can't add circular move at line %d, error code %d"),
//                    emcmotCommand->id, res_addcircle);
//        emcmotStatus->commandStatus = EMCMOT_COMMAND_BAD_EXEC;
//        tpAbort(&emcmotInternal->coord_tp);
//        SET_MOTION_ERROR_FLAG(1);
//        break;
//        } else if (res_addcircle != 0) {
//            //FIXME! This is a band-aid for a single issue, but there may be
//            //other consequences of non-fatal errors from AddXXX functions. We
//            //either need to fix the root cause (subtle position error after
//            //homing), or have a full restore here.
//            if (issue_atspeed) {
//                emcmotStatus->atspeed_next_feed = 1;
//            }
//        } else {
//        SET_MOTION_ERROR_FLAG(0);
//        /* set flag that indicates all joints need rehoming, if any
//           joint is moved in joint mode, for machines with no forward
//           kins */
//        rehomeAll = 1;
//        }
//        break;

//    case EMCMOT_SET_VEL:
//        /* set the velocity for subsequent moves */
//        /* can do it at any time */
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_VEL");
//        emcmotStatus->vel = emcmotCommand->vel;
//        tpSetVmax(&emcmotInternal->coord_tp, emcmotStatus->vel, emcmotCommand->ini_maxvel);
//        break;

//    case EMCMOT_SET_VEL_LIMIT:
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_VEL_LIMIT");
//        emcmot_config_change();
//        /* set the absolute max velocity for all subsequent moves */
//        /* can do it at any time */
//        emcmotConfig->limitVel = emcmotCommand->vel;
//        tpSetVlimit(&emcmotInternal->coord_tp, emcmotConfig->limitVel);
//        break;

//    case EMCMOT_SET_JOINT_VEL_LIMIT:
//        /* set joint max velocity */
//        /* can do it at any time */
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_JOINT_VEL_LIMIT");
//        rtapi_print_msg(RTAPI_MSG_DBG, " %d", joint_num);
//        emcmot_config_change();
//        /* check joint range */
//        if (joint == 0) {
//        break;
//        }
//        joint->vel_limit = emcmotCommand->vel;
//        break;

//    case EMCMOT_SET_JOINT_ACC_LIMIT:
//        /* set joint max acceleration */
//        /* can do it at any time */
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_JOINT_ACC_LIMIT");
//        rtapi_print_msg(RTAPI_MSG_DBG, " %d", joint_num);
//        emcmot_config_change();
//        /* check joint range */
//        if (joint == 0) {
//        break;
//        }
//        joint->acc_limit = emcmotCommand->acc;
//        break;

//    case EMCMOT_SET_ACC:
//        /* set the max acceleration */
//        /* can do it at any time */
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_ACCEL");
//        emcmotStatus->acc = emcmotCommand->acc;
//        tpSetAmax(&emcmotInternal->coord_tp, emcmotStatus->acc);
//        break;

//    case EMCMOT_PAUSE:
//        /* pause the motion */
//        /* can happen at any time */
//        rtapi_print_msg(RTAPI_MSG_DBG, "PAUSE");
//        tpPause(&emcmotInternal->coord_tp);
//        emcmotStatus->paused = 1;
//        break;

//    case EMCMOT_REVERSE:
//        /* run motion in reverse*/
//        /* only allowed during a pause */
//        rtapi_print_msg(RTAPI_MSG_DBG, "REVERSE");
//        tpSetRunDir(&emcmotInternal->coord_tp, TC_DIR_REVERSE);
//        break;

//    case EMCMOT_FORWARD:
//        /* run motion in reverse*/
//        /* only allowed during a pause */
//        rtapi_print_msg(RTAPI_MSG_DBG, "FORWARD");
//        tpSetRunDir(&emcmotInternal->coord_tp, TC_DIR_FORWARD);
//        break;

//    case EMCMOT_RESUME:
//        /* resume paused motion */
//        /* can happen at any time */
//        rtapi_print_msg(RTAPI_MSG_DBG, "RESUME");
//        emcmotStatus->stepping = 0;
//        tpResume(&emcmotInternal->coord_tp);
//        emcmotStatus->paused = 0;
//        break;

//    case EMCMOT_STEP:
//        /* resume paused motion until id changes */
//        /* can happen at any time */
//            rtapi_print_msg(RTAPI_MSG_DBG, "STEP");
//            if(emcmotStatus->paused) {
//                emcmotInternal->idForStep = emcmotStatus->id;
//                emcmotStatus->stepping = 1;
//                tpResume(&emcmotInternal->coord_tp);
//                emcmotStatus->paused = 1;
//            } else {
//        reportError(_("MOTION: can't STEP while already executing"));
//        }
//        break;

//    case EMCMOT_FEED_SCALE:
//        /* override speed */
//        /* can happen at any time */
//        rtapi_print_msg(RTAPI_MSG_DBG, "FEED SCALE");
//        if (emcmotCommand->scale < 0.0) {
//        emcmotCommand->scale = 0.0;	/* clamp it */
//        }
//        emcmotStatus->feed_scale = emcmotCommand->scale;
//        break;

//    case EMCMOT_RAPID_SCALE:
//        /* override rapids */
//        /* can happen at any time */
//        rtapi_print_msg(RTAPI_MSG_DBG, "RAPID SCALE");
//        if (emcmotCommand->scale < 0.0) {
//        emcmotCommand->scale = 0.0;	/* clamp it */
//        }
//        emcmotStatus->rapid_scale = emcmotCommand->scale;
//        break;

//    case EMCMOT_FS_ENABLE:
//        /* enable/disable overriding speed */
//        /* can happen at any time */
//        if ( emcmotCommand->mode != 0 ) {
//        rtapi_print_msg(RTAPI_MSG_DBG, "FEED SCALE: ON");
//        emcmotStatus->enables_new |= FS_ENABLED;
//            } else {
//        rtapi_print_msg(RTAPI_MSG_DBG, "FEED SCALE: OFF");
//        emcmotStatus->enables_new &= ~FS_ENABLED;
//        }
//        break;

//    case EMCMOT_FH_ENABLE:
//        /* enable/disable feed hold */
//        /* can happen at any time */
//        if ( emcmotCommand->mode != 0 ) {
//        rtapi_print_msg(RTAPI_MSG_DBG, "FEED HOLD: ENABLED");
//        emcmotStatus->enables_new |= FH_ENABLED;
//            } else {
//        rtapi_print_msg(RTAPI_MSG_DBG, "FEED HOLD: DISABLED");
//        emcmotStatus->enables_new &= ~FH_ENABLED;
//        }
//        break;

//    case EMCMOT_SPINDLE_SCALE:
//        /* override spindle speed */
//        /* can happen at any time */
//        rtapi_print_msg(RTAPI_MSG_DBG, "SPINDLE SCALE");
//        if (emcmotCommand->scale < 0.0) {
//        emcmotCommand->scale = 0.0;	/* clamp it */
//        }
//        emcmotStatus->spindle_status[emcmotCommand->spindle].scale = emcmotCommand->scale;
//        break;

//    case EMCMOT_SS_ENABLE:
//        /* enable/disable overriding spindle speed */
//        /* can happen at any time */
//        if ( emcmotCommand->mode != 0 ) {
//        rtapi_print_msg(RTAPI_MSG_DBG, "SPINDLE SCALE: ON");
//        emcmotStatus->enables_new |= SS_ENABLED;
//            } else {
//        rtapi_print_msg(RTAPI_MSG_DBG, "SPINDLE SCALE: OFF");
//        emcmotStatus->enables_new &= ~SS_ENABLED;
//        }
//        break;

//    case EMCMOT_AF_ENABLE:
//        /* enable/disable adaptive feedrate override from HAL pin */
//        /* can happen at any time */
//        if ( emcmotCommand->flags != 0 ) {
//        rtapi_print_msg(RTAPI_MSG_DBG, "ADAPTIVE FEED: ON");
//        emcmotStatus->enables_new |= AF_ENABLED;
//            } else {
//        rtapi_print_msg(RTAPI_MSG_DBG, "ADAPTIVE FEED: OFF");
//        emcmotStatus->enables_new &= ~AF_ENABLED;
//        }
//        break;

//    case EMCMOT_DISABLE:
//        /* go into disable */
//        /* can happen at any time */
//        /* reset the emcmotInternal->enabling flag to defer disable until
//           controller cycle (it *will* be honored) */
//        rtapi_print_msg(RTAPI_MSG_DBG, "DISABLE");
//        emcmotInternal->enabling = 0;
//        if (emcmotConfig->kinType == KINEMATICS_INVERSE_ONLY) {
//        emcmotInternal->teleoperating = 0;
//        emcmotInternal->coordinating = 0;
//        }
//        break;

//    case EMCMOT_ENABLE:
//        /* come out of disable */
//        /* can happen at any time */
//        /* set the emcmotInternal->enabling flag to defer enable until
//           controller cycle */
//        rtapi_print_msg(RTAPI_MSG_DBG, "ENABLE");
//        if ( *(emcmot_hal_data->enable) == 0 ) {
//        reportError(_("can't enable motion, enable input is false"));
//        } else {
//        emcmotInternal->enabling = 1;
//        if (emcmotConfig->kinType == KINEMATICS_INVERSE_ONLY) {
//            emcmotInternal->teleoperating = 0;
//            emcmotInternal->coordinating = 0;
//        }
//        }
//        break;

//    case EMCMOT_JOINT_ACTIVATE:
//        /* make joint active, so that amps will be enabled when system is
//           enabled or disabled */
//        /* can be done at any time */
//        rtapi_print_msg(RTAPI_MSG_DBG, "JOINT_ACTIVATE");
//        rtapi_print_msg(RTAPI_MSG_DBG, " %d", joint_num);
//        if (joint == 0) {
//        break;
//        }
//        SET_JOINT_ACTIVE_FLAG(joint, 1);
//        break;

//    case EMCMOT_JOINT_DEACTIVATE:
//        /* make joint inactive, so that amps won't be affected when system
//           is enabled or disabled */
//        /* can be done at any time */
//        rtapi_print_msg(RTAPI_MSG_DBG, "JOINT_DEACTIVATE");
//        rtapi_print_msg(RTAPI_MSG_DBG, " %d", joint_num);
//        if (joint == 0) {
//        break;
//        }
//        SET_JOINT_ACTIVE_FLAG(joint, 0);
//        break;

//    case EMCMOT_JOINT_HOME:
//        /* home the specified joint */
//        /* need to be in free mode, enable on */
//        /* this just sets the initial state, then the state machine in
//           homing.c does the rest */
//        rtapi_print_msg(RTAPI_MSG_DBG, "JOINT_HOME");
//        rtapi_print_msg(RTAPI_MSG_DBG, " %d", joint_num);

//        if (emcmotStatus->motion_state != EMCMOT_MOTION_FREE) {
//        /* can't home unless in free mode */
//        reportError(_("must be in joint mode to home"));
//        return;
//        }
//        if (*(emcmot_hal_data->homing_inhibit)) {
//            reportError(_("Homing denied by motion.homing-inhibit joint=%d\n"),
//                       joint_num);
//                return;
//        }

//        if (!GET_MOTION_ENABLE_FLAG()) {
//        break;
//        }

//        // Negative joint_num specifies homeall
//        do_home_joint(joint_num);
//        break;

//    case EMCMOT_JOINT_UNHOME:
//            /* unhome the specified joint, or all joints if -1 */
//            rtapi_print_msg(RTAPI_MSG_DBG, "JOINT_UNHOME");
//            rtapi_print_msg(RTAPI_MSG_DBG, " %d", joint_num);

//            if (   (emcmotStatus->motion_state != EMCMOT_MOTION_FREE)
//                && (emcmotStatus->motion_state != EMCMOT_MOTION_DISABLED)) {
//                reportError(_("must be in joint mode or disabled to unhome"));
//                return;
//            }

//            //Negative joint_num specifies unhome_method (-1,-2)
//            set_unhomed(joint_num,emcmotStatus->motion_state);
//            break;

//    case EMCMOT_CLEAR_PROBE_FLAGS:
//        rtapi_print_msg(RTAPI_MSG_DBG, "CLEAR_PROBE_FLAGS");
//        emcmotStatus->probing = 0;
//            emcmotStatus->probeTripped = 0;
//        break;

//    case EMCMOT_PROBE:
//        /* most of this is taken from EMCMOT_SET_LINE */
//        /* emcmotInternal->coord_tp up a linear move */
//        /* requires coordinated mode, enable off, not on limits */
//        rtapi_print_msg(RTAPI_MSG_DBG, "PROBE");
//        if (!GET_MOTION_COORD_FLAG() || !GET_MOTION_ENABLE_FLAG()) {
//        reportError(_("need to be enabled, in coord mode for probe move"));
//        emcmotStatus->commandStatus = EMCMOT_COMMAND_INVALID_COMMAND;
//        SET_MOTION_ERROR_FLAG(1);
//        break;
//        } else if (!inRange(emcmotCommand->pos, emcmotCommand->id, "Probe")) {
//        emcmotStatus->commandStatus = EMCMOT_COMMAND_INVALID_PARAMS;
//        tpAbort(&emcmotInternal->coord_tp);
//        SET_MOTION_ERROR_FLAG(1);
//        break;
//        } else if (!limits_ok()) {
//        reportError(_("can't do probe move with limits exceeded"));
//        emcmotStatus->commandStatus = EMCMOT_COMMAND_INVALID_PARAMS;
//        tpAbort(&emcmotInternal->coord_tp);
//        SET_MOTION_ERROR_FLAG(1);
//        break;
//        } else if (!(emcmotCommand->probe_type & 1)) {
//                // if suppress errors = off...

//                int probeval = !!*(emcmot_hal_data->probe_input);
//                int probe_whenclears = !!(emcmotCommand->probe_type & 2);

//                if (probeval != probe_whenclears) {
//                    // the probe is already in the state we're seeking.
//                    if(probe_whenclears)
//                        reportError(_("Probe is already clear when starting G38.4 or G38.5 move"));
//                    else
//                        reportError(_("Probe is already tripped when starting G38.2 or G38.3 move"));

//                    emcmotStatus->commandStatus = EMCMOT_COMMAND_BAD_EXEC;
//                    tpAbort(&emcmotInternal->coord_tp);
//                    SET_MOTION_ERROR_FLAG(1);
//                    break;
//                }
//            }

//        /* append it to the emcmotInternal->coord_tp */
//        tpSetId(&emcmotInternal->coord_tp, emcmotCommand->id);
//        if (-1 == tpAddLine(&emcmotInternal->coord_tp,
//                emcmotCommand->pos,
//                emcmotCommand->motion_type,
//                emcmotCommand->vel,
//                emcmotCommand->ini_maxvel,
//                emcmotCommand->acc,
//                emcmotStatus->enables_new,
//                0,
//                -1,
//                emcmotCommand->tag)) {
//        reportError(_("can't add probe move"));
//        emcmotStatus->commandStatus = EMCMOT_COMMAND_BAD_EXEC;
//        tpAbort(&emcmotInternal->coord_tp);
//        SET_MOTION_ERROR_FLAG(1);
//        break;
//        } else {
//        emcmotStatus->probing = 1;
//                emcmotStatus->probe_type = emcmotCommand->probe_type;
//        SET_MOTION_ERROR_FLAG(0);
//        /* set flag that indicates all joints need rehoming, if any
//           joint is moved in joint mode, for machines with no forward
//           kins */
//        rehomeAll = 1;
//        }
//        break;

//    case EMCMOT_RIGID_TAP:
//        /* most of this is taken from EMCMOT_SET_LINE */
//        /* emcmotInternal->coord_tp up a linear move */
//        /* requires coordinated mode, enable off, not on limits */
//        rtapi_print_msg(RTAPI_MSG_DBG, "RIGID_TAP");
//        if (!GET_MOTION_COORD_FLAG() || !GET_MOTION_ENABLE_FLAG()) {
//        reportError(_("need to be enabled, in coord mode for rigid tap move"));
//        emcmotStatus->commandStatus = EMCMOT_COMMAND_INVALID_COMMAND;
//        SET_MOTION_ERROR_FLAG(1);
//        break;
//        } else if (!inRange(emcmotCommand->pos, emcmotCommand->id, "Rigid tap")) {
//        emcmotStatus->commandStatus = EMCMOT_COMMAND_INVALID_PARAMS;
//        tpAbort(&emcmotInternal->coord_tp);
//        SET_MOTION_ERROR_FLAG(1);
//        break;
//        } else if (!limits_ok()) {
//        reportError(_("can't do rigid tap move with limits exceeded"));
//        emcmotStatus->commandStatus = EMCMOT_COMMAND_INVALID_PARAMS;
//        tpAbort(&emcmotInternal->coord_tp);
//        SET_MOTION_ERROR_FLAG(1);
//        break;
//        }

//        /* append it to the emcmotInternal->tp */
//        tpSetId(&emcmotInternal->coord_tp, emcmotCommand->id);
//        int res_addtap = tpAddRigidTap(&emcmotInternal->coord_tp,
//                                    emcmotCommand->pos,
//                                    emcmotCommand->vel,
//                                    emcmotCommand->ini_maxvel,
//                                    emcmotCommand->acc,
//                                    emcmotStatus->enables_new,
//                                    emcmotCommand->scale,
//                                    emcmotCommand->tag);
//        if (res_addtap < 0) {
//            emcmotStatus->atspeed_next_feed = 0; /* rigid tap always waits for spindle to be at-speed */
//            reportError(_("can't add rigid tap move at line %d, error code %d"),
//                    emcmotCommand->id, res_addtap);
//        tpAbort(&emcmotInternal->coord_tp);
//        SET_MOTION_ERROR_FLAG(1);
//        break;
//        } else {
//        SET_MOTION_ERROR_FLAG(0);
//        }
//        break;

//    case EMCMOT_SET_DEBUG:
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_DEBUG");
//        emcmotConfig->debug = emcmotCommand->debug;
//        emcmot_config_change();
//        break;

//    /* needed for synchronous I/O */
//    case EMCMOT_SET_AOUT:
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_AOUT");
//        if (emcmotCommand->now) { //we set it right away
//        emcmotAioWrite(emcmotCommand->out, emcmotCommand->minLimit);
//        } else { // we put it on the TP queue, warning: only room for one in there, any new ones will overwrite
//        tpSetAout(&emcmotInternal->coord_tp, emcmotCommand->out,
//            emcmotCommand->minLimit, emcmotCommand->maxLimit);
//        }
//        break;

//    case EMCMOT_SET_DOUT:
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_DOUT");
//        if (emcmotCommand->now) { //we set it right away
//        emcmotDioWrite(emcmotCommand->out, emcmotCommand->start);
//        } else { // we put it on the TP queue, warning: only room for one in there, any new ones will overwrite
//        tpSetDout(&emcmotInternal->coord_tp, emcmotCommand->out,
//            emcmotCommand->start, emcmotCommand->end);
//        }
//        break;

//    case EMCMOT_SET_SPINDLE_PARAMS:
//        rtapi_print_msg(RTAPI_MSG_DBG, "SPINDLE_SETUP: spindle %d/%d max_pos %f min_pos %f"
//                "max_neg %f min_neg %f, home: %f, %f, %d\n",
//                        emcmotCommand->spindle, emcmotConfig->numSpindles, emcmotCommand->maxLimit,
//                        emcmotCommand->min_pos_speed, emcmotCommand->max_neg_speed, emcmotCommand->minLimit,
//                        emcmotCommand->search_vel, emcmotCommand->home, emcmotCommand->home_sequence);
//        spindle_num = emcmotCommand->spindle;
//        if (spindle_num >= emcmotConfig->numSpindles){
//            reportError(_("Attempt to configure non-existent spindle"));
//            emcmotStatus->commandStatus = EMCMOT_COMMAND_INVALID_COMMAND;
//            break;
//        }
//        emcmotStatus->spindle_status[spindle_num].max_pos_speed = emcmotCommand->maxLimit;
//        emcmotStatus->spindle_status[spindle_num].min_neg_speed = emcmotCommand->minLimit;
//        emcmotStatus->spindle_status[spindle_num].max_neg_speed = emcmotCommand->max_neg_speed;
//        emcmotStatus->spindle_status[spindle_num].min_pos_speed = emcmotCommand->min_pos_speed;
//        emcmotStatus->spindle_status[spindle_num].home_search_vel = emcmotCommand->search_vel;
//        emcmotStatus->spindle_status[spindle_num].home_sequence = emcmotCommand->home_sequence;
//        emcmotStatus->spindle_status[spindle_num].increment = emcmotCommand->offset;

//        break;
//    case EMCMOT_SPINDLE_ON:
//        rtapi_print_msg(RTAPI_MSG_DBG, "SPINDLE_ON: spindle %d/%d speed %d\n",
//                        emcmotCommand->spindle, emcmotConfig->numSpindles, (int) emcmotCommand->vel);
//        spindle_num = emcmotCommand->spindle;
//        if (spindle_num >= emcmotConfig->numSpindles){
//            reportError(_("Attempt to start non-existent spindle"));
//            emcmotStatus->commandStatus = EMCMOT_COMMAND_INVALID_COMMAND;
//            break;
//        }
//        s0 = spindle_num;
//        s1 = spindle_num;
//        if (spindle_num ==-1){
//            s0 = 0;
//            s1 = motion_num_spindles - 1;
//        }
//        for (n = s0; n<=s1; n++){

//            if (*(emcmot_hal_data->spindle[n].spindle_orient))
//            rtapi_print_msg(RTAPI_MSG_DBG, "SPINDLE_ORIENT cancelled by SPINDLE_ON\n");
//            if (*(emcmot_hal_data->spindle[n].spindle_locked))
//            rtapi_print_msg(RTAPI_MSG_DBG, "spindle-locked cleared by SPINDLE_ON\n");
//            *(emcmot_hal_data->spindle[n].spindle_locked) = 0;
//            *(emcmot_hal_data->spindle[n].spindle_orient) = 0;
//            emcmotStatus->spindle_status[n].orient_state = EMCMOT_ORIENT_NONE;

//            /* if (emcmotStatus->spindle.orient) { */
//            /* 	reportError(_("can\'t turn on spindle during orient in progress")); */
//            /* 	emcmotStatus->commandStatus = EMCMOT_COMMAND_INVALID_COMMAND; */
//            /* 	tpAbort(&emcmotInternal->tp); */
//            /* 	SET_MOTION_ERROR_FLAG(1); */
//            /* } else {...} */
//            rtapi_print_msg(RTAPI_MSG_DBG, "command state %d\n", emcmotCommand->state);
//            emcmotStatus->spindle_status[n].state = emcmotCommand->state;
//            emcmotStatus->spindle_status[n].speed = emcmotCommand->vel;
//            emcmotStatus->spindle_status[n].css_factor = emcmotCommand->ini_maxvel;
//            emcmotStatus->spindle_status[n].xoffset = emcmotCommand->acc;
//            if (emcmotCommand->state) {
//                if (emcmotCommand->vel >= 0) {
//                emcmotStatus->spindle_status[n].direction = 1;
//                } else {
//                emcmotStatus->spindle_status[n].direction = -1;
//                }
//                emcmotStatus->spindle_status[n].brake = 0; //disengage brake
//            }
//            apply_spindle_limits(&emcmotStatus->spindle_status[n]);
//        }
//        emcmotStatus->atspeed_next_feed = emcmotCommand->wait_for_spindle_at_speed;

//       // check whether it's passed correctly
//       if (!emcmotStatus->atspeed_next_feed){
//           rtapi_print_msg(RTAPI_MSG_DBG, "SPINDLE_ON without wait-for-atspeed");
//       }
//       break;

//    case EMCMOT_SPINDLE_OFF:
//        rtapi_print_msg(RTAPI_MSG_DBG, "SPINDLE_OFF");
//        spindle_num = emcmotCommand->spindle;
//        if (spindle_num >= emcmotConfig->numSpindles){
//            reportError(_("Attempt to stop non-existent spindle <%d>"),spindle_num);
//            emcmotStatus->commandStatus = EMCMOT_COMMAND_INVALID_COMMAND;
//            break;
//        }
//        s0 = spindle_num;
//        s1 = spindle_num;
//        if (spindle_num ==-1){
//            s0 = 0;
//            s1 = motion_num_spindles - 1;
//        }
//        for (n = s0; n<=s1; n++){

//            emcmotStatus->spindle_status[n].state = 0;
//            emcmotStatus->spindle_status[n].speed = 0;
//            emcmotStatus->spindle_status[n].direction = 0;
//            emcmotStatus->spindle_status[n].brake = 1; // engage brake
//            if (*(emcmot_hal_data->spindle[n].spindle_orient))
//            rtapi_print_msg(RTAPI_MSG_DBG, "SPINDLE_ORIENT cancelled by SPINDLE_OFF");
//            if (*(emcmot_hal_data->spindle[n].spindle_locked)){
//            rtapi_print_msg(RTAPI_MSG_DBG, "spindle-locked cleared by SPINDLE_OFF");
//                *(emcmot_hal_data->spindle[n].spindle_locked) = 0;
//            }
//            *(emcmot_hal_data->spindle[n].spindle_orient) = 0;
//            emcmotStatus->spindle_status[n].orient_state = EMCMOT_ORIENT_NONE;
//        }
//        break;

//    case EMCMOT_SPINDLE_ORIENT:
//        rtapi_print_msg(RTAPI_MSG_DBG, "SPINDLE_ORIENT");
//        spindle_num = emcmotCommand->spindle;
//        if (spindle_num >= emcmotConfig->numSpindles){
//            reportError(_("Attempt to orient non-existent spindle <%d>"),spindle_num);
//            emcmotStatus->commandStatus = EMCMOT_COMMAND_INVALID_COMMAND;
//            break;
//        }
//        s0 = spindle_num;
//        s1 = spindle_num;
//        if (spindle_num ==-1){
//            s0 = 0;
//            s1 = motion_num_spindles - 1;
//        }
//        for (n = s0; n<=s1; n++){

//            if (n > emcmotConfig->numSpindles){
//                rtapi_print_msg(RTAPI_MSG_ERR, "spindle number <%d> too high in M19",n);
//                break;
//            }
//            if (*(emcmot_hal_data->spindle[n].spindle_orient)) {
//            rtapi_print_msg(RTAPI_MSG_DBG, "orient already in progress");

//            // mah:FIXME unsure whether this is ok or an error
//            /* reportError(_("orient already in progress")); */
//            /* emcmotStatus->commandStatus = EMCMOT_COMMAND_INVALID_COMMAND; */
//            /* tpAbort(&emcmotInternal->tp); */
//            /* SET_MOTION_ERROR_FLAG(1); */
//            }
//            emcmotStatus->spindle_status[n].orient_state = EMCMOT_ORIENT_IN_PROGRESS;
//            emcmotStatus->spindle_status[n].speed = 0;
//            emcmotStatus->spindle_status[n].direction = 0;
//            // so far like spindle stop, except opening brake
//            emcmotStatus->spindle_status[n].brake = 0; // open brake

//            // https://github.com/LinuxCNC/linuxcnc/issues/3389
//            emcmotStatus->spindle_status[n].state = 0;
//            *(emcmot_hal_data->spindle[n].spindle_on) = 0;
//            // https://github.com/LinuxCNC/linuxcnc/issues/3389

//            *(emcmot_hal_data->spindle[n].spindle_orient_angle) = emcmotCommand->orientation;
//            *(emcmot_hal_data->spindle[n].spindle_orient_mode) = emcmotCommand->mode;
//            *(emcmot_hal_data->spindle[n].spindle_locked) = 0;
//            *(emcmot_hal_data->spindle[n].spindle_orient) = 1;

//            // mirror in spindle status
//            emcmotStatus->spindle_status[n].orient_fault = 0; // this pin read during spindle-orient == 1
//            emcmotStatus->spindle_status[n].locked = 0;
//        }
//        break;

//    case EMCMOT_SPINDLE_INCREASE:
//        rtapi_print_msg(RTAPI_MSG_DBG, "SPINDLE_INCREASE");
//        spindle_num = emcmotCommand->spindle;
//        if (spindle_num >= emcmotConfig->numSpindles){
//            reportError(_("Attempt to increase non-existent spindle <%d>"),spindle_num);
//            emcmotStatus->commandStatus = EMCMOT_COMMAND_INVALID_COMMAND;
//            break;
//        }
//        s0 = spindle_num;
//        s1 = spindle_num;
//        if (spindle_num ==-1){
//            s0 = 0;
//            s1 = motion_num_spindles - 1;
//        }
//        for (n = s0; n<=s1; n++){
//            if (emcmotStatus->spindle_status[n].speed > 0) {
//            emcmotStatus->spindle_status[n].speed += emcmotStatus->spindle_status[n].increment;
//            } else if (emcmotStatus->spindle_status[n].speed < 0) {
//            emcmotStatus->spindle_status[n].speed -= emcmotStatus->spindle_status[n].increment;
//            }
//            apply_spindle_limits(&emcmotStatus->spindle_status[n]);
//        }
//        break;

//    case EMCMOT_SPINDLE_DECREASE:
//        rtapi_print_msg(RTAPI_MSG_DBG, "SPINDLE_DECREASE");
//        spindle_num = emcmotCommand->spindle;
//        if (spindle_num >= emcmotConfig->numSpindles){
//            reportError(_("Attempt to decrease non-existent spindle <%d>."),spindle_num);
//            emcmotStatus->commandStatus = EMCMOT_COMMAND_INVALID_COMMAND;
//            break;
//        }
//        s0 = spindle_num;
//        s1 = spindle_num;
//        if (spindle_num ==-1){
//            s0 = 0;
//            s1 = motion_num_spindles - 1;
//        }
//        for (n = s0; n<=s1; n++){
//            if (emcmotStatus->spindle_status[n].speed > emcmotStatus->spindle_status[n].increment) {
//                emcmotStatus->spindle_status[n].speed -= emcmotStatus->spindle_status[n].increment;
//            } else if (emcmotStatus->spindle_status[n].speed < -1.0 * emcmotStatus->spindle_status[n].increment) {
//                emcmotStatus->spindle_status[n].speed += emcmotStatus->spindle_status[n].increment;
//            }
//            apply_spindle_limits(&emcmotStatus->spindle_status[n]);
//        }
//        break;

//    case EMCMOT_SPINDLE_BRAKE_ENGAGE:
//        rtapi_print_msg(RTAPI_MSG_DBG, "SPINDLE_BRAKE_ENGAGE");
//        spindle_num = emcmotCommand->spindle;
//        if (spindle_num >= emcmotConfig->numSpindles){
//            reportError(_("Attempt to engage brake of non-existent spindle <%d>"),spindle_num);
//            emcmotStatus->commandStatus = EMCMOT_COMMAND_INVALID_COMMAND;
//            break;
//        }
//        s0 = spindle_num;
//        s1 = spindle_num;
//        if (spindle_num ==-1){
//            s0 = 0;
//            s1 = motion_num_spindles - 1;
//        }
//        for (n = s0; n<=s1; n++){

//            emcmotStatus->spindle_status[n].speed = 0;
//            emcmotStatus->spindle_status[n].direction = 0;
//            emcmotStatus->spindle_status[n].brake = 1;
//        }
//        break;

//    case EMCMOT_SPINDLE_BRAKE_RELEASE:
//        rtapi_print_msg(RTAPI_MSG_DBG, "SPINDLE_BRAKE_RELEASE");
//        spindle_num = emcmotCommand->spindle;
//        if (spindle_num >= emcmotConfig->numSpindles){
//            reportError(_("Attempt to release brake of non-existent spindle <%d>"),spindle_num);
//            emcmotStatus->commandStatus = EMCMOT_COMMAND_INVALID_COMMAND;
//            break;
//        }
//        s0 = spindle_num;
//        s1 = spindle_num;
//        if (spindle_num ==-1){
//            s0 = 0;
//            s1 = motion_num_spindles - 1;
//        }
//        for (n = s0; n<=s1; n++){

//            emcmotStatus->spindle_status[n].brake = 0;
//        }
//        break;

//    case EMCMOT_SET_JOINT_COMP:
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_JOINT_COMP for joint %d", joint_num);
//        if (joint == 0) {
//        break;
//        }
//        if (joint->comp.entries >= EMCMOT_COMP_SIZE) {
//        reportError(_("joint %d: too many compensation entries"), joint_num);
//        break;
//        }
//        /* point to last entry */
//        comp_entry = &(joint->comp.array[joint->comp.entries]);
//        if (emcmotCommand->comp_nominal <= comp_entry[0].nominal) {
//        reportError(_("joint %d: compensation values must increase"), joint_num);
//        break;
//        }
//        /* store data to new entry */
//        comp_entry[1].nominal = emcmotCommand->comp_nominal;
//        comp_entry[1].fwd_trim = emcmotCommand->comp_forward;
//        comp_entry[1].rev_trim = emcmotCommand->comp_reverse;
//        /* calculate slopes from previous entry to the new one */
//        if ( comp_entry[0].nominal != -DBL_MAX ) {
//        /* but only if the previous entry is "real" */
//        tmp1 = comp_entry[1].nominal - comp_entry[0].nominal;
//        comp_entry[0].fwd_slope =
//            (comp_entry[1].fwd_trim - comp_entry[0].fwd_trim) / tmp1;
//        comp_entry[0].rev_slope =
//            (comp_entry[1].rev_trim - comp_entry[0].rev_trim) / tmp1;
//        } else {
//        /* previous entry is at minus infinity, slopes are zero */
//        comp_entry[0].fwd_trim = comp_entry[1].fwd_trim;
//        comp_entry[0].rev_trim = comp_entry[1].rev_trim;
//        }
//        joint->comp.entries++;
//        break;

//        case EMCMOT_SET_OFFSET:
//            emcmotStatus->tool_offset = emcmotCommand->tool_offset;
//            break;

//    case EMCMOT_SET_AXIS_POSITION_LIMITS:
//        /* set the position limits for axis */
//        /* can be done at any time */
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_AXIS_POSITION_LIMITS");
//        rtapi_print_msg(RTAPI_MSG_DBG, " %d", emcmotCommand->axis);
//        emcmot_config_change();
//            if ((emcmotCommand->axis < 0) || (emcmotCommand->axis >= EMCMOT_MAX_AXIS)) {
//                break;
//            }
//            axis_set_min_pos_limit(emcmotCommand->axis, emcmotCommand->minLimit);
//            axis_set_max_pos_limit(emcmotCommand->axis, emcmotCommand->maxLimit);
//        break;

//        case EMCMOT_SET_AXIS_VEL_LIMIT:
//        /* set the max axis vel */
//        /* can be done at any time */
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_AXIS_VEL_LIMITS");
//        rtapi_print_msg(RTAPI_MSG_DBG, " %d", emcmotCommand->axis);
//        emcmot_config_change();
//            if ((emcmotCommand->axis < 0) || (emcmotCommand->axis >= EMCMOT_MAX_AXIS)) {
//                break;
//            }
//            axis_set_vel_limit(emcmotCommand->axis, emcmotCommand->vel);
//            axis_set_ext_offset_vel_limit(emcmotCommand->axis, emcmotCommand->ext_offset_vel);
//            break;

//        case EMCMOT_SET_AXIS_ACC_LIMIT:
//        /* set the max axis acc */
//        /* can be done at any time */
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_AXIS_ACC_LIMITS");
//        rtapi_print_msg(RTAPI_MSG_DBG, " %d", emcmotCommand->axis);
//        emcmot_config_change();
//            if ((emcmotCommand->axis < 0) || (emcmotCommand->axis >= EMCMOT_MAX_AXIS)) {
//                break;
//            }
//            axis_set_acc_limit(emcmotCommand->axis, emcmotCommand->acc);
//            axis_set_ext_offset_acc_limit(emcmotCommand->axis, emcmotCommand->ext_offset_acc);
//            break;

//        case EMCMOT_SET_AXIS_LOCKING_JOINT:
//        rtapi_print_msg(RTAPI_MSG_DBG, "SET_AXIS_ACC_LOCKING_JOINT");
//        rtapi_print_msg(RTAPI_MSG_DBG, " %d", emcmotCommand->axis);
//        emcmot_config_change();
//            if ((emcmotCommand->axis < 0) || (emcmotCommand->axis >= EMCMOT_MAX_AXIS)) {
//                break;
//            }
//            axis_set_locking_joint(emcmotCommand->axis, joint_num);
//            break;

//    default:
//        rtapi_print_msg(RTAPI_MSG_DBG, "UNKNOWN");
//        reportError(_("unrecognized command %d"), emcmotCommand->command);
//        emcmotStatus->commandStatus = EMCMOT_COMMAND_UNKNOWN_COMMAND;
//        break;
//        case EMCMOT_SET_MAX_FEED_OVERRIDE:
//            emcmotConfig->maxFeedScale = emcmotCommand->maxFeedScale;
//            break;
//        case EMCMOT_SETUP_ARC_BLENDS:
//            emcmotConfig->arcBlendEnable = emcmotCommand->arcBlendEnable;
//            emcmotConfig->arcBlendFallbackEnable = emcmotCommand->arcBlendFallbackEnable;
//            emcmotConfig->arcBlendOptDepth = emcmotCommand->arcBlendOptDepth;
//            emcmotConfig->arcBlendGapCycles = emcmotCommand->arcBlendGapCycles;
//            emcmotConfig->arcBlendRampFreq = emcmotCommand->arcBlendRampFreq;
//            emcmotConfig->arcBlendTangentKinkRatio = emcmotCommand->arcBlendTangentKinkRatio;
//            break;
//        case EMCMOT_SET_PROBE_ERR_INHIBIT:
//            emcmotConfig->inhibit_probe_jog_error = emcmotCommand->probe_jog_err_inhibit;
//            emcmotConfig->inhibit_probe_home_error = emcmotCommand->probe_home_err_inhibit;
//            break;

//    }			/* end of: command switch */
//    if (emcmotStatus->commandStatus != EMCMOT_COMMAND_OK) {
//        rtapi_print_msg(RTAPI_MSG_DBG, "ERROR: %d",
//        emcmotStatus->commandStatus);
//    }
//    rtapi_print_msg(RTAPI_MSG_DBG, "\n");
//    /* synch tail count */
//    emcmotStatus->tail = emcmotStatus->head;
//    emcmotConfig->tail = emcmotConfig->head;
//    emcmotInternal->tail = emcmotInternal->head;

//    }
//    /* end of: if-new-command */

//    return;
//}


extern int rtapi_app_main_kines(void);
extern int rtapi_app_main_motion(void);
void MotionTask::InitMotion()
{
    rtapi_app_main_kines();
    rtapi_app_main_motion();
}

static long servo_period = 1000000;
extern void emcmotCommandHandler(void *arg, long servo_period);
void MotionTask::CmdHandler()
{
    emcmotCommandHandler(NULL, servo_period);
//    rtapi_app_main_kines();
}
