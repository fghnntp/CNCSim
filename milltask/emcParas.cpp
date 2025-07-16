#include "emcParas.h"
#include "emc.hh"
//#include "rs274ngc_interp.hh"	// the interpreter
#include "emcIniFile.hh"
#include "emcglb.h"
//#include "interpl.hh"
#include <iostream>
#include "emcIniFile.hh"
//#include "mot_priv.h"
#include "motion.h"
#include "emcChannel.h"
#include "emccfg.h"
//#include "usrmotintf.h"
#include "homing.h"

/* define this to catch isnan errors, for rtlinux FPU register
   problem testing */
#define ISNAN_TRAP

#ifdef ISNAN_TRAP
#define CATCH_NAN(cond) do {                           \
    if (cond) {                                        \
        printf("isnan error in %s()\n", __FUNCTION__); \
        return -1;                                     \
    }                                                  \
} while(0)
#else
#define CATCH_NAN(cond) do {} while(0)
#endif

extern void InitonUserMotionIF(void);
extern void InitTaskinft(void);
extern TrajConfig_t *GetTrajConfig();
extern JointConfig_t *GetJointConfig(int joint);
extern AxisConfig_t *GetAxisConfig(int axis);
extern SpindleConfig_t *GetSpindleConfig(int spindle);

std::string EMCParas::inifileName;
EMCParas::MotTrajConfig EMCParas::trajconfig;
EMCParas::MotJointConfig EMCParas::jointconfig[EMCMOT_MAX_JOINTS];
EMCParas::MotAxisConfig EMCParas::axisconfig[EMCMOT_MAX_AXIS];

void EMCParas::init_emc_paras()
{
    // This is motion simulation init and task simulation init
    // Just malloc memory, not finally used
    emcStatus->motion.traj.axis_mask=~0;
    InitonUserMotionIF();
    InitTaskinft();

    // Paras need to load for init moduels
    // Traj Kins Joint Axis
    if (inifileName != "") {
        EmcIniFile infile;
        if (!infile.Open(inifileName.c_str())) {
            return;
        }
        iniTraj(inifileName.c_str());
        for (int joint = 0; joint < GetTrajConfig()->Joints; joint++) {
            iniJoint(joint, inifileName.c_str());
        }
        for (int axis = 0; axis < EMCMOT_MAX_AXIS; axis++) {
              if (GetTrajConfig()->AxisMask & (1<<axis)) {
              if (0 != iniAxis(axis, inifileName.c_str())) {
                    printf("%s: emcAxisInit(%d) failed\n", __FUNCTION__, axis);
              }
            }
        }
        for (int spindle = 0; spindle < GetTrajConfig()->Spindles; spindle++) {
               if (0 != iniSpindle(spindle, inifileName.c_str())) {
                    printf("%s: emcSpindleInit(%d) failed\n", __FUNCTION__, spindle);
               }
        }
    }
}

void EMCParas::set_traj_maxvel(double vel)
{
    GetTrajConfig()->MaxVel = vel;
}

/*
  loadKins()

  JOINTS <int>                  number of joints (DOF) in system

  calls:

  emcTrajSetJoints(int joints);
*/

int EMCParas::loadKins(EmcIniFile *trajInifile)
{
    trajInifile->EnableExceptions(EmcIniFile::ERR_CONVERSION);

    try {
    int joints = 0;
    trajInifile->Find(&joints, "JOINTS", "KINS");

        if (0 != emcTrajSetJoints_(joints)) {
            return -1;
        }
    }

    catch (EmcIniFile::Exception &e) {
        e.Print();
        return -1;
    }

    return 0;
}


/*
  loadTraj()

  Loads INI file params for traj

  COORDINATES <char[]>            axes in system
  LINEAR_UNITS <float>            units per mm
  ANGULAR_UNITS <float>           units per degree
  DEFAULT_LINEAR_VELOCITY <float> default linear velocity
  MAX_LINEAR_VELOCITY <float>     max linear velocity
  DEFAULT_LINEAR_ACCELERATION <float> default linear acceleration
  MAX_LINEAR_ACCELERATION <float>     max linear acceleration

  calls:

  emcTrajSetAxes(int axes, int axismask);
  emcTrajSetUnits(double linearUnits, double angularUnits);
  emcTrajSetVelocity(double vel, double ini_maxvel);
  emcTrajSetAcceleration(double acc);
  emcTrajSetMaxVelocity(double vel);
  emcTrajSetMaxAcceleration(double acc);
  */
int EMCParas::loadTraj(EmcIniFile *trajInifile)
{
    EmcLinearUnits linearUnits;
    EmcAngularUnits angularUnits;
    double vel;
    double acc;
    double traj_default_velocity;

    trajInifile->EnableExceptions(EmcIniFile::ERR_CONVERSION);

    try{
        int spindles = 1;
        trajInifile->Find(&spindles, "SPINDLES", "TRAJ");
        if (0 != emcTrajSetSpindles_(spindles)) {
            return -1;
        }
    }

    catch (EmcIniFile::Exception &e) {
        e.Print();
        return -1;
    }

    try{
    int axismask = 0;
    const char *coord = trajInifile->Find("COORDINATES", "TRAJ");
    if(coord) {
        if(strchr(coord, 'x') || strchr(coord, 'X')) {
             axismask |= 1;
            }
        if(strchr(coord, 'y') || strchr(coord, 'Y')) {
             axismask |= 2;
            }
        if(strchr(coord, 'z') || strchr(coord, 'Z')) {
             axismask |= 4;
            }
        if(strchr(coord, 'a') || strchr(coord, 'A')) {
             axismask |= 8;
            }
        if(strchr(coord, 'b') || strchr(coord, 'B')) {
             axismask |= 16;
            }
        if(strchr(coord, 'c') || strchr(coord, 'C')) {
             axismask |= 32;
            }
        if(strchr(coord, 'u') || strchr(coord, 'U')) {
             axismask |= 64;
            }
        if(strchr(coord, 'v') || strchr(coord, 'V')) {
             axismask |= 128;
            }
        if(strchr(coord, 'w') || strchr(coord, 'W')) {
             axismask |= 256;
            }
    } else {
        axismask = 1 | 2 | 4;		// default: XYZ machine
    }
        if (0 != emcTrajSetAxes_(axismask)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                printf("bad return value from emcTrajSetAxes\n");
            }
            return -1;
        }

        linearUnits = 0;
        trajInifile->FindLinearUnits(&linearUnits, "LINEAR_UNITS", "TRAJ");
        angularUnits = 0;
        trajInifile->FindAngularUnits(&angularUnits, "ANGULAR_UNITS", "TRAJ");
        if (0 != emcTrajSetUnits_(linearUnits, angularUnits)) {
            printf("emcTrajSetUnits failed to set "
                      "[TRAJ]LINEAR_UNITS or [TRAJ]ANGULAR_UNITS\n");
            return -1;
        }

        vel = 1.0;
        trajInifile->Find(&vel, "DEFAULT_LINEAR_VELOCITY", "TRAJ");
        trajconfig.traj_default_velocity = vel;

        // and set dynamic value
        if (0 != emcTrajSetVelocity_(0, vel)) { //default velocity on startup 0
            if (emc_debug & EMC_DEBUG_CONFIG) {
                printf("bad return value from emcTrajSetVelocity\n");
            }
            return -1;
        }

        vel = 1e99; // by default, use AXIS limit
        trajInifile->Find(&vel, "MAX_LINEAR_VELOCITY", "TRAJ");
        // XXX CJR merge question: Set something in TrajConfig here?
        trajconfig.traj_max_velocity = vel;

        // and set dynamic value
        if (0 != emcTrajSetMaxVelocity_(vel)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                printf("bad return value from emcTrajSetMaxVelocity\n");
            }
            return -1;
        }

        acc = 1e99; // let the axis values apply
        trajInifile->Find(&acc, "DEFAULT_LINEAR_ACCELERATION", "TRAJ");
        if (0 != emcTrajSetAcceleration_(acc)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                printf("bad return value from emcTrajSetAcceleration\n");
            }
            return -1;
        }
        trajconfig.traj_default_acceleration = acc;

        acc = 1e99; // let the axis values apply
        trajInifile->Find(&acc, "MAX_LINEAR_ACCELERATION", "TRAJ");
        if (0 != emcTrajSetMaxAcceleration_(acc)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                printf("bad return value from emcTrajSetMaxAcceleration\n");
            }
            return -1;
        }
        trajconfig.traj_max_acceleration = acc;

        int arcBlendEnable = 1;
        int arcBlendFallbackEnable = 0;
        int arcBlendOptDepth = 50;
        int arcBlendGapCycles = 4;
        double arcBlendRampFreq = 100.0;
        double arcBlendTangentKinkRatio = 0.1;

        trajInifile->Find(&arcBlendEnable, "ARC_BLEND_ENABLE", "TRAJ");
        trajInifile->Find(&arcBlendFallbackEnable, "ARC_BLEND_FALLBACK_ENABLE", "TRAJ");
        trajInifile->Find(&arcBlendOptDepth, "ARC_BLEND_OPTIMIZATION_DEPTH", "TRAJ");
        trajInifile->Find(&arcBlendGapCycles, "ARC_BLEND_GAP_CYCLES", "TRAJ");
        trajInifile->Find(&arcBlendRampFreq, "ARC_BLEND_RAMP_FREQ", "TRAJ");
        trajInifile->Find(&arcBlendTangentKinkRatio, "ARC_BLEND_KINK_RATIO", "TRAJ");

        if (0 != emcSetupArcBlends_(arcBlendEnable, arcBlendFallbackEnable,
                    arcBlendOptDepth, arcBlendGapCycles, arcBlendRampFreq, arcBlendTangentKinkRatio)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                printf("bad return value from emcSetupArcBlends\n");
            }
            return -1;
        }

        trajconfig.traj_arc_blend_enable = arcBlendEnable;
        trajconfig.traj_arc_blend_fallback_enable = arcBlendFallbackEnable;
        trajconfig.traj_arc_blend_optimization_depth = arcBlendOptDepth;
        trajconfig.traj_arc_blend_gap_cycles = arcBlendGapCycles;
        trajconfig.traj_arc_blend_ramp_freq = arcBlendRampFreq;
        trajconfig.traj_arc_blend_tangent_kink_ratio = arcBlendTangentKinkRatio;
        //TODO update inihal

        double maxFeedScale = 1.0;
        trajInifile->Find(&maxFeedScale, "MAX_FEED_OVERRIDE", "DISPLAY");

        if (0 != emcSetMaxFeedOverride_(maxFeedScale)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                printf("bad return value from emcSetMaxFeedOverride\n");
            }
            return -1;
        }

        int j_inhibit = 0;
        int h_inhibit = 0;
        trajInifile->Find(&j_inhibit, "NO_PROBE_JOG_ERROR", "TRAJ");
        trajInifile->Find(&h_inhibit, "NO_PROBE_HOME_ERROR", "TRAJ");
        if (0 != emcSetProbeErrorInhibit_(j_inhibit, h_inhibit)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                printf("bad return value from emcSetProbeErrorInhibit\n");
            }
            return -1;
        }
    }

    catch (EmcIniFile::Exception &e) {
        e.Print();
        return -1;
    }
    try{
        const char *inistring;
        unsigned char coordinateMark[6] = { 1, 1, 1, 0, 0, 0 };
        int t;
        int len;
        char homes[LINELEN];
        char home[LINELEN];
        EmcPose homePose = { {0.0, 0.0, 0.0}, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
        double d;
        if (NULL != (inistring = trajInifile->Find("HOME", "TRAJ"))) {
            // [TRAJ]HOME is important for genhexkins.c kinetmaticsForward()
            // and probably other non-identity kins that solve the forward
            // kinematics with an iterative algorithm when the homePose
            // is not all zeros

            // found it, now interpret it according to coordinateMark[]
            rtapi_strxcpy(homes, inistring);
            len = 0;
            for (t = 0; t < 6; t++) {
                if (!coordinateMark[t]) {
                    continue;    // position t at index of next non-zero mark
                }
                // there is a mark, so read the string for a value
                if (1 == sscanf(&homes[len], "%254s", home) &&
                    1 == sscanf(home, "%lf", &d)) {
                    // got an entry, index into coordinateMark[] is 't'
                    if (t == 0)
                        homePose.tran.x = d;
                    else if (t == 1)
                        homePose.tran.y = d;
                    else if (t == 2)
                        homePose.tran.z = d;
                    else if (t == 3)
                        homePose.a = d;
                    else if (t == 4)
                        homePose.b = d;
                    else if (t == 5)
                        homePose.c = d;
/*
 * The following have no effect. The loop only counts [0..5].
                    else if (t == 6)
                        homePose.u = d;
                    else if (t == 7)
                        homePose.v = d;
                    else
                        homePose.w = d;
*/

                    // position string ptr past this value
                    len += strlen(home);
                    // and at start of next value
                    while ((len < LINELEN) && (homes[len] == ' ' || homes[len] == '\t')) {
                        len++;
                    }
                    if (len >= LINELEN) {
                        break;    // out of for loop
                    }
                } else {
                    // badly formatted entry
                    printf("invalid INI file value for [TRAJ] HOME: %s\n",
                          inistring);
                        return -1;
                }
            }  // end of for-loop on coordinateMark[]
        }
        if (0 != emcTrajSetHome(homePose)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                printf("bad return value from emcTrajSetHome\n");
            }
            return -1;
        }
     } //try

    catch (EmcIniFile::Exception &e) {
        e.Print();
        return -1;
    }
    return 0;
}

/*
  iniTraj(const char *filename)

  Loads INI file parameters for trajectory, from [TRAJ] section
 */
int EMCParas::iniTraj(const char *filename)
{
    EmcIniFile trajInifile;

    if (trajInifile.Open(filename) == false) {
    return -1;
    }
    // load trajectory values
    if (0 != loadKins(&trajInifile)) {
    return -1;
    }
    // load trajectory values
    if (0 != loadTraj(&trajInifile)) {
    return -1;
    }

    GetTrajConfig()->Inited = 1;

    return 0;
}


int EMCParas::emcTrajSetJoints_(int joints)
{
    if (joints <= 0 || joints > EMCMOT_MAX_JOINTS) {
    printf("emcTrajSetJoints failing: joints=%d\n",
        joints);
    return -1;
    }

    GetTrajConfig()->Joints = joints;
    EMCChannel::emcmotCommand.command = EMCMOT_SET_NUM_JOINTS;
    EMCChannel::emcmotCommand.joint = joints;
    EMCChannel::push();
    return 0;
}

int EMCParas::emcTrajSetUnits_(double linearUnits, double angularUnits)
{
    if (linearUnits <= 0.0 || angularUnits <= 0.0) {
    return -1;
    }

    GetTrajConfig()->LinearUnits = linearUnits;
    GetTrajConfig()->AngularUnits = angularUnits;

    if (emc_debug & EMC_DEBUG_CONFIG) {
        printf("%s(%.4f, %.4f)\n", __FUNCTION__, linearUnits, angularUnits);
    }
    return 0;
}

int EMCParas::emcTrajSetSpindles_(int spindles)
{
    if (spindles <= 0 || spindles > EMCMOT_MAX_SPINDLES) {
    printf("emcTrajSetSpindles failing: spindles=%d\n",
        spindles);
    return -1;
    }

    GetTrajConfig()->Spindles = spindles;
    EMCChannel::emcmotCommand.command = EMCMOT_SET_NUM_SPINDLES;
    EMCChannel::emcmotCommand.spindle = spindles;
    EMCChannel::push();

    return 0;
}

int EMCParas::emcTrajSetAxes_(int axismask)
{
    int axes = 0;
    for(int i=0; i<EMCMOT_MAX_AXIS; i++)
        if(axismask & (1<<i)) axes = i+1;

    GetTrajConfig()->AxisMask = axismask;

    if (emc_debug & EMC_DEBUG_CONFIG) {
        printf("%s(%d, %d)\n", __FUNCTION__, axes, axismask);
    }
    return 0;
}

int EMCParas::emcTrajSetVelocity_(double vel, double ini_maxvel)
{
    if (vel < 0.0) {
    vel = 0.0;
    } else if (vel > GetTrajConfig()->MaxVel) {
    vel = GetTrajConfig()->MaxVel;
    }

    if (ini_maxvel < 0.0) {
        ini_maxvel = 0.0;
    } else if (vel > GetTrajConfig()->MaxVel) {
        ini_maxvel = GetTrajConfig()->MaxVel;
    }

    EMCChannel::emcmotCommand.command = EMCMOT_SET_VEL;
    EMCChannel::emcmotCommand.vel = vel;
    EMCChannel::emcmotCommand.ini_maxvel = ini_maxvel;

    EMCChannel::push();

    return 0;
}


/*
  emcmot has no limits on max velocity, acceleration so we'll save them
  here and apply them in the functions above
  */
int EMCParas::emcTrajSetMaxVelocity_(double vel)
{
    if (vel < 0.0) {
    vel = 0.0;
    }

    GetTrajConfig()->MaxVel = vel;

    EMCChannel::emcmotCommand.command = EMCMOT_SET_VEL_LIMIT;
    EMCChannel::emcmotCommand.vel = vel;

    EMCChannel::push();

    return 0;
}

int EMCParas::emcTrajSetAcceleration_(double acc)
{
    if (acc < 0.0) {
    acc = 0.0;
    } else if (acc > GetTrajConfig()->MaxAccel) {
    acc = GetTrajConfig()->MaxAccel;
    }

    EMCChannel::emcmotCommand.command = EMCMOT_SET_ACC;
    EMCChannel::emcmotCommand.acc = acc;

    EMCChannel::push();

    return 0;
}

int EMCParas::emcTrajSetMaxAcceleration_(double acc)
{
    if (acc < 0.0) {
    acc = 0.0;
    }

    GetTrajConfig()->MaxAccel = acc;

    if (emc_debug & EMC_DEBUG_CONFIG) {
        printf("%s(%.4g)\n", __FUNCTION__, acc);
    }
    return 0;
}

int EMCParas::emcSetupArcBlends_(int arcBlendEnable,
                        int arcBlendFallbackEnable,
                        int arcBlendOptDepth,
                        int arcBlendGapCycles,
                        double arcBlendRampFreq,
                        double arcBlendTangentKinkRatio)
{
    EMCChannel::emcmotCommand.command = EMCMOT_SETUP_ARC_BLENDS;
    EMCChannel::emcmotCommand.arcBlendEnable = arcBlendEnable;
    EMCChannel::emcmotCommand.arcBlendFallbackEnable = arcBlendFallbackEnable;
    EMCChannel::emcmotCommand.arcBlendOptDepth = arcBlendOptDepth;
    EMCChannel::emcmotCommand.arcBlendGapCycles = arcBlendGapCycles;
    EMCChannel::emcmotCommand.arcBlendRampFreq = arcBlendRampFreq;
    EMCChannel::emcmotCommand.arcBlendTangentKinkRatio = arcBlendTangentKinkRatio;
    EMCChannel::push();
    return 0;
}

int EMCParas::emcSetMaxFeedOverride_(double maxFeedScale) {
    EMCChannel::emcmotCommand.command = EMCMOT_SET_MAX_FEED_OVERRIDE;
    EMCChannel::emcmotCommand.maxFeedScale = maxFeedScale;
    EMCChannel::push();

    return 0;
}

int EMCParas::emcSetProbeErrorInhibit_(int j_inhibit, int h_inhibit) {
    EMCChannel::emcmotCommand.command = EMCMOT_SET_PROBE_ERR_INHIBIT;
    EMCChannel::emcmotCommand.probe_jog_err_inhibit = j_inhibit;
    EMCChannel::emcmotCommand.probe_home_err_inhibit = h_inhibit;
    EMCChannel::push();

    return 0;
}

int EMCParas::emcTrajSetHome(const EmcPose& home)
{
#ifdef ISNAN_TRAP
    if (std::isnan(home.tran.x) || std::isnan(home.tran.y) || std::isnan(home.tran.z) ||
    std::isnan(home.a) || std::isnan(home.b) || std::isnan(home.c) ||
    std::isnan(home.u) || std::isnan(home.v) || std::isnan(home.w)) {
    printf("std::isnan error in emcTrajSetHome()\n");
    return 0;		// ignore it for now, just don't send it
    }
#endif

    EMCChannel::emcmotCommand.command = EMCMOT_SET_WORLD_HOME;
    EMCChannel::emcmotCommand.pos = home;
    EMCChannel::push();

    return 0;
}

/*
  iniJoint(int joint, const char *filename)

  Loads INI file parameters for specified joint

  Looks for [KINS]JOINTS for how many to do, up to EMC_JOINT_MAX.
 */
int EMCParas::iniJoint(int joint, const char *filename)
{
    EmcIniFile jointIniFile(EmcIniFile::ERR_TAG_NOT_FOUND |
                           EmcIniFile::ERR_SECTION_NOT_FOUND |
                           EmcIniFile::ERR_CONVERSION);

    if (jointIniFile.Open(filename) == false) {
    return -1;
    }

    // load its values
    if (0 != loadJoint(joint, &jointIniFile)) {
        return -1;
    }

    GetJointConfig(joint)->Inited = 1;

    return 0;
}

/*
  loadJoint(int joint)

  Loads INI file params for joint, joint = 0, ...

  TYPE <LINEAR ANGULAR>        type of joint
  MAX_VELOCITY <float>         max vel for joint
  MAX_ACCELERATION <float>     max accel for joint
  BACKLASH <float>             backlash
  MIN_LIMIT <float>            minimum soft position limit
  MAX_LIMIT <float>            maximum soft position limit
  FERROR <float>               maximum following error, scaled to max vel
  MIN_FERROR <float>           minimum following error
  HOME <float>                 home position (where to go after home)
  HOME_FINAL_VEL <float>       speed to move from HOME_OFFSET to HOME location (at the end of homing)
  HOME_OFFSET <float>          home switch/index pulse location
  HOME_SEARCH_VEL <float>      homing speed, search phase
  HOME_LATCH_VEL <float>       homing speed, latch phase
  HOME_USE_INDEX <bool>        use index pulse when homing
  HOME_IGNORE_LIMITS <bool>    ignore limit switches when homing
  COMP_FILE <filename>         file of joint compensation points

  calls:

  emcJointSetType(int joint, unsigned char jointType);
  emcJointSetUnits(int joint, double units);
  emcJointSetBacklash(int joint, double backlash);
  emcJointSetMinPositionLimit(int joint, double limit);
  emcJointSetMaxPositionLimit(int joint, double limit);
  emcJointSetFerror(int joint, double ferror);
  emcJointSetMinFerror(int joint, double ferror);
  emcJointSetHomingParams(int joint, double home, double offset, double home_vel,
                          double search_vel, double latch_vel,
                          int use_index, int encoder_does_not_reset,
                          int ignore_limits, int is_shared, int sequence, int volatile_home));
  emcJointActivate(int joint);
  emcJointSetMaxVelocity(int joint, double vel);
  emcJointSetMaxAcceleration(int joint, double acc);
  emcJointLoadComp(int joint, const char * file, int comp_file_type);
  */

int EMCParas::loadJoint(int joint, EmcIniFile *jointIniFile)
{
    char jointString[16];
    const char *inistring;
    EmcJointType jointType;
    double units;
    double backlash;
    double offset;
    double limit;
    double home;
    double search_vel;
    double latch_vel;
    double final_vel; // moving from OFFSET to HOME
    bool use_index;
    bool encoder_does_not_reset;
    bool ignore_limits;
    bool is_shared;
    int sequence;
    int volatile_home;
    int locking_indexer;
    int absolute_encoder;
    int comp_file_type; //type for the compensation file. type==0 means nom, forw, rev.
    double maxVelocity;
    double maxAcceleration;
    double ferror;

    // compose string to match, joint = 0 -> JOINT_0, etc.
    snprintf(jointString, sizeof(jointString), "JOINT_%d", joint);

    jointIniFile->EnableExceptions(EmcIniFile::ERR_CONVERSION);

    try {
        // set joint type
        jointType = EMC_LINEAR;	// default
        jointIniFile->Find(&jointType, "TYPE", jointString);
        if (0 != emcJointSetType_(joint, jointType)) {
            return -1;
        }

        // set units
        if(jointType == EMC_LINEAR){
            units = emcTrajGetLinearUnits();
        }else{
            units = emcTrajGetAngularUnits();
        }
        if (0 != emcJointSetUnits_(joint, units)) {
            return -1;
        }

        // set backlash
        backlash = 0;	                // default
        jointIniFile->Find(&backlash, "BACKLASH", jointString);
        if (0 != emcJointSetBacklash_(joint, backlash)) {
            return -1;
        }
        jointconfig[joint].joint_backlash = backlash;

        // set min position limit
        limit = -1e99;	                // default
        jointIniFile->Find(&limit, "MIN_LIMIT", jointString);
        if (0 != emcJointSetMinPositionLimit_(joint, limit)) {
             return -1;
        }
        jointconfig[joint].joint_min_limit = limit;

        // set max position limit
        limit = 1e99;	                // default
        jointIniFile->Find(&limit, "MAX_LIMIT", jointString);
        if (0 != emcJointSetMaxPositionLimit_(joint, limit)) {
            return -1;
        }
        jointconfig[joint].joint_max_limit = limit;

        // set following error limit (at max speed)
        ferror = 1;	                // default
        jointIniFile->Find(&ferror, "FERROR", jointString);
        if (0 != emcJointSetFerror_(joint, ferror)) {
             return -1;
        }
        jointconfig[joint].joint_ferror = ferror;

        // do MIN_FERROR, if it's there. If not, use value of maxFerror above
        jointIniFile->Find(&ferror, "MIN_FERROR", jointString);
        if (0 != emcJointSetMinFerror_(joint, ferror)) {
            return -1;
        }
        jointconfig[joint].joint_min_ferror = ferror;

        // set homing paramsters
        home = 0;	                // default
        jointIniFile->Find(&home, "HOME", jointString);
        jointconfig[joint].joint_home = home;

        offset = 0;	                // default
        jointIniFile->Find(&offset, "HOME_OFFSET", jointString);
        jointconfig[joint].joint_home_offset = offset;

        search_vel = 0;	                // default
        jointIniFile->Find(&search_vel, "HOME_SEARCH_VEL", jointString);
        latch_vel = 0;	                // default
        jointIniFile->Find(&latch_vel, "HOME_LATCH_VEL", jointString);
        final_vel = -1;	                // default (rapid)
        jointIniFile->Find(&final_vel, "HOME_FINAL_VEL", jointString);
        is_shared = false;	        // default
        jointIniFile->Find(&is_shared, "HOME_IS_SHARED", jointString);
        use_index = false;	        // default
        jointIniFile->Find(&use_index, "HOME_USE_INDEX", jointString);
        encoder_does_not_reset = false;	// default
        jointIniFile->Find(&encoder_does_not_reset, "HOME_INDEX_NO_ENCODER_RESET", jointString);
        ignore_limits = false;	        // default
        jointIniFile->Find(&ignore_limits, "HOME_IGNORE_LIMITS", jointString);

        sequence = 999;// default: use unrealizable and positive sequence no.
                       // so that joints with unspecified HOME_SEQUENCE=
                       // will not be homed in home-all
        jointIniFile->Find(&sequence, "HOME_SEQUENCE", jointString);
        jointconfig[joint].joint_home_sequence = sequence;

        volatile_home = 0;	        // default
        jointIniFile->Find(&volatile_home, "VOLATILE_HOME", jointString);
        locking_indexer = false;
        jointIniFile->Find(&locking_indexer, "LOCKING_INDEXER", jointString);
        absolute_encoder = false;
        jointIniFile->Find(&absolute_encoder, "HOME_ABSOLUTE_ENCODER", jointString);
        // issue NML message to set all params
        if (0 != emcJointSetHomingParams_(joint, home, offset
                                        ,final_vel, search_vel, latch_vel
                                        ,(int)use_index
                                        ,(int)encoder_does_not_reset
                                        ,(int)ignore_limits
                                        ,(int)is_shared
                                        ,sequence
                                        ,volatile_home
                                        ,locking_indexer
                                        ,absolute_encoder
                                        )) {
            return -1;
        }

        // set maximum velocity
        maxVelocity = DEFAULT_JOINT_MAX_VELOCITY;
        jointIniFile->Find(&maxVelocity, "MAX_VELOCITY", jointString);
        if (0 != emcJointSetMaxVelocity_(joint, maxVelocity)) {
            return -1;
        }
        jointconfig[joint].joint_max_velocity = maxVelocity;

        maxAcceleration = DEFAULT_JOINT_MAX_ACCELERATION;
        jointIniFile->Find(&maxAcceleration, "MAX_ACCELERATION", jointString);
        if (0 != emcJointSetMaxAcceleration_(joint, maxAcceleration)) {
            return -1;
        }
        jointconfig[joint].joint_max_acceleration = maxAcceleration;

        comp_file_type = 0;             // default
        jointIniFile->Find(&comp_file_type, "COMP_FILE_TYPE", jointString);
        if (NULL != (inistring = jointIniFile->Find("COMP_FILE", jointString))) {
            if (0 != emcJointLoadComp(joint, inistring, comp_file_type)) {
                return -1;
            }
        }
    }

    catch (EmcIniFile::Exception &e) {
        e.Print();
        return -1;
    }

    // lastly, activate joint. Do this last so that the motion controller
    // won't flag errors midway during configuration
    if (0 != emcJointActivate_(joint)) {
        return -1;
    }

    return 0;
}

// axes and joints are numbered 0..NUM-1

/*
  In emcmot, we need to set the cycle time for traj, and the interpolation
  rate, in any order, but both need to be done.
 */

/*! functions involving joints */

int EMCParas::emcJointSetType_(int joint, unsigned char jointType)
{
    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }

    GetJointConfig(joint)->Type = jointType;

    if (emc_debug & EMC_DEBUG_CONFIG) {
        printf("%s(%d, %d)\n", __FUNCTION__, joint, jointType);
    }
    return 0;
}

int EMCParas::emcJointSetUnits_(int joint, double units)
{
    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }

    GetJointConfig(joint)->Units = units;

    if (emc_debug & EMC_DEBUG_CONFIG) {
        printf("%s(%d, %.4f)\n", __FUNCTION__, joint, units);
    }
    return 0;
}

int EMCParas::emcJointSetBacklash_(int joint, double backlash)
{
#ifdef ISNAN_TRAP
    if (std::isnan(backlash)) {
    printf("std::isnan error in emcJointSetBacklash()\n");
    return -1;
    }
#endif

    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }

    EMCChannel::emcmotCommand.command = EMCMOT_SET_JOINT_BACKLASH;
    EMCChannel::emcmotCommand.joint = joint;
    EMCChannel::emcmotCommand.backlash = backlash;

    EMCChannel::push();

    return 0;
}

int EMCParas::emcJointSetMinPositionLimit_(int joint, double limit)
{
#ifdef ISNAN_TRAP
    if (std::isnan(limit)) {
    printf("isnan error in emcJointSetMinPosition()\n");
    return -1;
    }
#endif

    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }

    GetJointConfig(joint)->MinLimit = limit;

    EMCChannel::emcmotCommand.command = EMCMOT_SET_JOINT_POSITION_LIMITS;
    EMCChannel::emcmotCommand.joint = joint;
    EMCChannel::emcmotCommand.minLimit = GetJointConfig(joint)->MinLimit;
    EMCChannel::emcmotCommand.maxLimit = GetJointConfig(joint)->MaxLimit;

    EMCChannel::push();

    return 0;
}

int EMCParas::emcJointSetMaxPositionLimit_(int joint, double limit)
{
#ifdef ISNAN_TRAP
    if (std::isnan(limit)) {
    printf("std::isnan error in emcJointSetMaxPosition()\n");
    return -1;
    }
#endif

    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }

    GetJointConfig(joint)->MaxLimit = limit;

    EMCChannel::emcmotCommand.command = EMCMOT_SET_JOINT_POSITION_LIMITS;
    EMCChannel::emcmotCommand.joint = joint;
    EMCChannel::emcmotCommand.minLimit = GetJointConfig(joint)->MinLimit;
    EMCChannel::emcmotCommand.maxLimit = GetJointConfig(joint)->MaxLimit;

    EMCChannel::push();

    return 0;
}

int EMCParas::emcJointSetFerror_(int joint, double ferror)
{
#ifdef ISNAN_TRAP
    if (std::isnan(ferror)) {
    printf("isnan error in emcJointSetFerror()\n");
    return -1;
    }
#endif

    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }

    EMCChannel::emcmotCommand.command = EMCMOT_SET_JOINT_MAX_FERROR;
    EMCChannel::emcmotCommand.joint = joint;
    EMCChannel::emcmotCommand.maxFerror = ferror;

    EMCChannel::push();

    return 0;
}

int EMCParas::emcJointSetMinFerror_(int joint, double ferror)
{
#ifdef ISNAN_TRAP
    if (std::isnan(ferror)) {
    printf("isnan error in emcJointSetMinFerror()\n");
    return -1;
    }
#endif

    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }
    EMCChannel::emcmotCommand.command = EMCMOT_SET_JOINT_MIN_FERROR;
    EMCChannel::emcmotCommand.joint = joint;
    EMCChannel::emcmotCommand.minFerror = ferror;

    EMCChannel::push();

    return 0;
}

int EMCParas::emcJointSetHomingParams_(int joint, double home, double offset, double home_final_vel,
               double search_vel, double latch_vel,
               int use_index, int encoder_does_not_reset,
               int ignore_limits, int is_shared,
               int sequence,int volatile_home, int locking_indexer,int absolute_encoder)
{
#ifdef ISNAN_TRAP
    if (std::isnan(home) || std::isnan(offset) || std::isnan(home_final_vel) ||
    std::isnan(search_vel) || std::isnan(latch_vel)) {
    printf("isnan error in emcJointSetHomingParams()\n");
    return -1;
    }
#endif

    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }

    EMCChannel::emcmotCommand.command = EMCMOT_SET_JOINT_HOMING_PARAMS;
    EMCChannel::emcmotCommand.joint = joint;
    EMCChannel::emcmotCommand.home = home;
    EMCChannel::emcmotCommand.offset = offset;
    EMCChannel::emcmotCommand.home_final_vel = home_final_vel;
    EMCChannel::emcmotCommand.search_vel = search_vel;
    EMCChannel::emcmotCommand.latch_vel = latch_vel;
    EMCChannel::emcmotCommand.flags = 0;
    EMCChannel::emcmotCommand.home_sequence = sequence;
    EMCChannel::emcmotCommand.volatile_home = volatile_home;
    if (use_index) {
    EMCChannel::emcmotCommand.flags |= HOME_USE_INDEX;
    }
    if (encoder_does_not_reset) {
    EMCChannel::emcmotCommand.flags |= HOME_INDEX_NO_ENCODER_RESET;
    }
    if (ignore_limits) {
    EMCChannel::emcmotCommand.flags |= HOME_IGNORE_LIMITS;
    }
    if (is_shared) {
    EMCChannel::emcmotCommand.flags |= HOME_IS_SHARED;
    }
    if (locking_indexer) {
        EMCChannel::emcmotCommand.flags |= HOME_UNLOCK_FIRST;
    }
    if (absolute_encoder) {
        switch (absolute_encoder) {
          case 0: break;
          case 1: EMCChannel::emcmotCommand.flags |= HOME_ABSOLUTE_ENCODER;
                  EMCChannel::emcmotCommand.flags |= HOME_NO_REHOME;
                  break;
          case 2: EMCChannel::emcmotCommand.flags |= HOME_ABSOLUTE_ENCODER;
                  EMCChannel::emcmotCommand.flags |= HOME_NO_REHOME;
                  EMCChannel::emcmotCommand.flags |= HOME_NO_FINAL_MOVE;
                  break;
          default: fprintf(stderr,
                   "Unknown option for absolute_encoder <%d>",absolute_encoder);
                  break;
        }
    }

    EMCChannel::push();

    return 0;
}

int EMCParas::emcJointSetMaxVelocity_(int joint, double vel)
{
    CATCH_NAN(std::isnan(vel));

    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }

    if (vel < 0.0) {
    vel = 0.0;
    }

    GetJointConfig(joint)->MaxVel = vel;

    EMCChannel::emcmotCommand.command = EMCMOT_SET_JOINT_VEL_LIMIT;
    EMCChannel::emcmotCommand.joint = joint;
    EMCChannel::emcmotCommand.vel = vel;

    EMCChannel::push();

    return 0;
}

int EMCParas::emcJointSetMaxAcceleration_(int joint, double acc)
{
    CATCH_NAN(std::isnan(acc));

    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }
    if (acc < 0.0) {
    acc = 0.0;
    }
    GetJointConfig(joint)->MaxAccel = acc;
    //FIXME-AJ: need functions for setting the AXIS_MAX_ACCEL (either from the INI, or from kins..)
    EMCChannel::emcmotCommand.command = EMCMOT_SET_JOINT_ACC_LIMIT;
    EMCChannel::emcmotCommand.joint = joint;
    EMCChannel::emcmotCommand.acc = acc;

    EMCChannel::push();

    return 0;
}

int EMCParas::emcJointActivate_(int joint)
{
    if (joint < 0 || joint >= EMCMOT_MAX_JOINTS) {
    return 0;
    }

    EMCChannel::emcmotCommand.command = EMCMOT_JOINT_ACTIVATE;
    EMCChannel::emcmotCommand.joint = joint;

    EMCChannel::push();

    return 0;
}

/*
  iniAxis(int axis, const char *filename)

  Loads INI file parameters for specified axis, [0 .. AXES - 1]

 */
int EMCParas::iniAxis(int axis, const char *filename)
{
    EmcIniFile axisIniFile(EmcIniFile::ERR_TAG_NOT_FOUND |
                           EmcIniFile::ERR_SECTION_NOT_FOUND |
                           EmcIniFile::ERR_CONVERSION);

    if (axisIniFile.Open(filename) == false) {
    return -1;
    }

    // load its values
    if (0 != loadAxis(axis, &axisIniFile)) {
        return -1;
    }

    GetAxisConfig(axis)->Inited = 1;

    return 0;
}


extern double ext_offset_a_or_v_ratio[EMCMOT_MAX_AXIS]; // all zero

// default ratio or external offset velocity,acceleration
#define DEFAULT_A_OR_V_RATIO 0

/*
  loadAxis(int axis)

  Loads INI file params for axis, axis = X, Y, Z, A, B, C, U, V, W

  TYPE <LINEAR ANGULAR>        type of axis (hardcoded: X,Y,Z,U,V,W: LINEAR, A,B,C: ANGULAR)
  MAX_VELOCITY <float>         max vel for axis
  MAX_ACCELERATION <float>     max accel for axis
  MIN_LIMIT <float>            minimum soft position limit
  MAX_LIMIT <float>            maximum soft position limit

  calls:

  emcAxisSetMinPositionLimit(int axis, double limit);
  emcAxisSetMaxPositionLimit(int axis, double limit);
  emcAxisSetMaxVelocity(int axis, double vel, double ext_offset_vel);
  emcAxisSetMaxAcceleration(int axis, double acc, double ext_offset_acc);
  */

int EMCParas::loadAxis(int axis, EmcIniFile *axisIniFile)
{
    char axisString[16];
    double limit;
    double maxVelocity;
    double maxAcceleration;
    int    lockingjnum = -1; // -1 ==> locking joint not used

    // compose string to match, axis = 0 -> AXIS_X etc.
    switch (axis) {
    case 0: snprintf(axisString, sizeof(axisString), "AXIS_X"); break;
    case 1: snprintf(axisString, sizeof(axisString), "AXIS_Y"); break;
    case 2: snprintf(axisString, sizeof(axisString), "AXIS_Z"); break;
    case 3: snprintf(axisString, sizeof(axisString), "AXIS_A"); break;
    case 4: snprintf(axisString, sizeof(axisString), "AXIS_B"); break;
    case 5: snprintf(axisString, sizeof(axisString), "AXIS_C"); break;
    case 6: snprintf(axisString, sizeof(axisString), "AXIS_U"); break;
    case 7: snprintf(axisString, sizeof(axisString), "AXIS_V"); break;
    case 8: snprintf(axisString, sizeof(axisString), "AXIS_W"); break;
    }

    axisIniFile->EnableExceptions(EmcIniFile::ERR_CONVERSION);

    try {
        // set min position limit
        limit = -1e99;	                // default
        axisIniFile->Find(&limit, "MIN_LIMIT", axisString);
        if (0 != emcAxisSetMinPositionLimit_(axis, limit)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                printf("bad return from emcAxisSetMinPositionLimit\n");
            }
            return -1;
        }
        axisconfig[axis].axis_min_limit = limit;

        // set max position limit
        limit = 1e99;	                // default
        axisIniFile->Find(&limit, "MAX_LIMIT", axisString);
        if (0 != emcAxisSetMaxPositionLimit_(axis, limit)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                printf("bad return from emcAxisSetMaxPositionLimit\n");
            }
            return -1;
        }
        axisconfig[axis].axis_max_limit = limit;

        ext_offset_a_or_v_ratio[axis] = DEFAULT_A_OR_V_RATIO;
        axisIniFile->Find(&ext_offset_a_or_v_ratio[axis], "OFFSET_AV_RATIO", axisString);

#define REPLACE_AV_RATIO 0.1
#define MAX_AV_RATIO     0.9
        if (   (ext_offset_a_or_v_ratio[axis] < 0)
            || (ext_offset_a_or_v_ratio[axis] > MAX_AV_RATIO)
           ) {
           printf("\n   Invalid:[%s]OFFSET_AV_RATIO= %8.5f\n"
                             "   Using:  [%s]OFFSET_AV_RATIO= %8.5f\n",
                           axisString,ext_offset_a_or_v_ratio[axis],
                           axisString,REPLACE_AV_RATIO);
           ext_offset_a_or_v_ratio[axis] = REPLACE_AV_RATIO;
        }

        // set maximum velocities for axis: vel,ext_offset_vel
        maxVelocity = DEFAULT_AXIS_MAX_VELOCITY;
        axisIniFile->Find(&maxVelocity, "MAX_VELOCITY", axisString);
        if (0 != emcAxisSetMaxVelocity_(axis,
                   (1 - ext_offset_a_or_v_ratio[axis]) * maxVelocity,
                   (    ext_offset_a_or_v_ratio[axis]) * maxVelocity)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                printf("bad return from emcAxisSetMaxVelocity\n");
            }
            return -1;
        }
        axisconfig[axis].axis_max_velocity = maxVelocity;

        // set maximum accels for axis: acc,ext_offset_acc
        maxAcceleration = DEFAULT_AXIS_MAX_ACCELERATION;
        axisIniFile->Find(&maxAcceleration, "MAX_ACCELERATION", axisString);
        if (0 != emcAxisSetMaxAcceleration_(axis,
                    (1 - ext_offset_a_or_v_ratio[axis]) * maxAcceleration,
                    (    ext_offset_a_or_v_ratio[axis]) * maxAcceleration)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                printf("bad return from emcAxisSetMaxAcceleration\n");
            }
            return -1;
        }
        axisconfig[axis].axis_max_acceleration = maxAcceleration;

        axisIniFile->Find(&lockingjnum, "LOCKING_INDEXER_JOINT", axisString);
        if (0 != emcAxisSetLockingJoint_(axis, lockingjnum)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                printf("bad return from emcAxisSetLockingJoint\n");
            }
            return -1;
        }
    }


    catch(EmcIniFile::Exception &e){
        e.Print();
        return -1;
    }

    return 0;
}

/*! functions involving cartesian Axes (X,Y,Z,A,B,C,U,V,W) */

int EMCParas::emcAxisSetMinPositionLimit_(int axis, double limit)
{
    CATCH_NAN(std::isnan(limit));

    if (axis < 0 || axis >= EMCMOT_MAX_AXIS || !(GetTrajConfig()->AxisMask & (1 << axis))) {
    return 0;
    }

    GetAxisConfig(axis)->MinLimit = limit;

    EMCChannel::emcmotCommand.command = EMCMOT_SET_AXIS_POSITION_LIMITS;
    EMCChannel::emcmotCommand.axis = axis;
    EMCChannel::emcmotCommand.minLimit = GetAxisConfig(axis)->MinLimit;
    EMCChannel::emcmotCommand.maxLimit = GetAxisConfig(axis)->MaxLimit;

    EMCChannel::push();

    return 0;
}

int EMCParas::emcAxisSetMaxPositionLimit_(int axis, double limit)
{
    CATCH_NAN(std::isnan(limit));

    if (axis < 0 || axis >= EMCMOT_MAX_AXIS || !(GetTrajConfig()->AxisMask & (1 << axis))) {
    return 0;
    }

    GetAxisConfig(axis)->MaxLimit = limit;

    EMCChannel::emcmotCommand.command = EMCMOT_SET_AXIS_POSITION_LIMITS;
    EMCChannel::emcmotCommand.axis = axis;
    EMCChannel::emcmotCommand.minLimit = GetAxisConfig(axis)->MinLimit;
    EMCChannel::emcmotCommand.maxLimit = GetAxisConfig(axis)->MaxLimit;

    EMCChannel::push();

    return 0;
}

int EMCParas::emcAxisSetMaxVelocity_(int axis, double vel,double ext_offset_vel)
{
    CATCH_NAN(std::isnan(vel));

    if (axis < 0 || axis >= EMCMOT_MAX_AXIS || !(GetTrajConfig()->AxisMask & (1 << axis))) {
    return 0;
    }

    if (vel < 0.0) {
    vel = 0.0;
    }

    GetAxisConfig(axis)->MaxVel = vel;

    EMCChannel::emcmotCommand.command = EMCMOT_SET_AXIS_VEL_LIMIT;
    EMCChannel::emcmotCommand.axis = axis;
    EMCChannel::emcmotCommand.vel = vel;
    EMCChannel::emcmotCommand.ext_offset_vel = ext_offset_vel;

    EMCChannel::push();

    return 0;
}

int EMCParas::emcAxisSetMaxAcceleration_(int axis, double acc,double ext_offset_acc)
{
    CATCH_NAN(std::isnan(acc));

    if (axis < 0 || axis >= EMCMOT_MAX_AXIS || !(GetTrajConfig()->AxisMask & (1 << axis))) {
    return 0;
    }

    if (acc < 0.0) {
    acc = 0.0;
    }

    GetAxisConfig(axis)->MaxAccel = acc;

    EMCChannel::emcmotCommand.command = EMCMOT_SET_AXIS_ACC_LIMIT;
    EMCChannel::emcmotCommand.axis = axis;
    EMCChannel::emcmotCommand.acc = acc;
    EMCChannel::emcmotCommand.ext_offset_acc = ext_offset_acc;

    EMCChannel::push();

    return 0;
}

int EMCParas::emcAxisSetLockingJoint_(int axis, int joint)
{

    if (axis < 0 || axis >= EMCMOT_MAX_AXIS || !(GetTrajConfig()->AxisMask & (1 << axis))) {
    return 0;
    }

    if (joint < 0) {
    joint = -1;
    }

    EMCChannel::emcmotCommand.command = EMCMOT_SET_AXIS_LOCKING_JOINT;
    EMCChannel::emcmotCommand.axis    = axis;
    EMCChannel::emcmotCommand.joint   = joint;

    EMCChannel::push();

    return 0;
}

/*
  iniAxis(int axis, const char *filename)

  Loads INI file parameters for specified axis, [0 .. AXES - 1]

 */
int EMCParas::iniSpindle(int spindle, const char *filename)
{
    EmcIniFile spindleIniFile(EmcIniFile::ERR_TAG_NOT_FOUND |
                           EmcIniFile::ERR_SECTION_NOT_FOUND |
                           EmcIniFile::ERR_CONVERSION);

    if (spindleIniFile.Open(filename) == false) {
    return -1;
    }

    // load its values
    if (0 != loadSpindle(spindle, &spindleIniFile)) {
        return -1;
    }

    GetSpindleConfig(spindle)->Inited = 1;

    return 0;
}

int EMCParas::loadSpindle(int spindle, EmcIniFile *spindleIniFile)
{
    int num_spindles = 1;
    char spindleString[11];
    double fastest_pos = 1e99;
    double slowest_neg = 0;
    double slowest_pos = 0;
    double fastest_neg = -1e99;
    int home_sequence = 0;
    double search_vel = 0;
    double home_angle = 0;
    double increment = 100;
    double limit;

    spindleIniFile->EnableExceptions(EmcIniFile::ERR_CONVERSION);

    if (spindleIniFile->Find(&num_spindles, "SPINDLES", "TRAJ") < 0){
        num_spindles = 1; }
    if (spindle > num_spindles) return -1;

    snprintf(spindleString, sizeof(spindleString), "SPINDLE_%i", spindle);

    // set max positive speed limit
    if (spindleIniFile->Find(&limit, "MAX_FORWARD_VELOCITY", spindleString) == 0){
        fastest_pos = limit;
        fastest_neg = -1.0 * limit;
    }
    // set min positive speed limit
    if (spindleIniFile->Find(&limit, "MIN_FORWARD_VELOCITY", spindleString) == 0){
        slowest_pos = limit;
        slowest_neg = -1.0 * limit;
    }
    // set min negative speed limit
    if (spindleIniFile->Find(&limit, "MIN_REVERSE_VELOCITY", spindleString) == 0){
        slowest_neg = -1.0 * fabs(limit);
    }
    // set max negative speed limit
    if (spindleIniFile->Find(&limit, "MAX_REVERSE_VELOCITY", spindleString) == 0){
        fastest_neg = -1.0 * fabs(limit);
    }
    // set home sequence
    if (spindleIniFile->Find(&limit, "HOME_SEQUENCE", spindleString) == 0){
        home_sequence = (int)limit;
    }
    // set home velocity
    if (spindleIniFile->Find(&limit, "HOME_SEARCH_VELOCITY", spindleString) == 0){
        search_vel = (int)limit;
    }
    /* set home angle - I believe this is a bad idea - andypugh 30/12/21
    if (spindleIniFile->Find(&limit, "HOME", spindleString) >= 0){
        home_angle = (int)limit;
    }*/
    home_angle = 0;
    // set spindle increment
    if (spindleIniFile->Find(&limit, "INCREMENT", spindleString) == 0){
        increment = limit;
    }

    if (0 != emcSpindleSetParams_(spindle, fastest_pos, slowest_pos, slowest_neg,
        fastest_neg, search_vel, home_angle, home_sequence, increment)) {
        return -1;
    }
    return 0;
}

int EMCParas::emcSpindleSetParams_(int spindle, double max_pos, double min_pos, double max_neg,
               double min_neg, double search_vel, double home_angle, int sequence, double increment)
{

    if (spindle < 0 || spindle >= EMCMOT_MAX_SPINDLES) {
    return 0;
    }

    EMCChannel::emcmotCommand.command = EMCMOT_SET_SPINDLE_PARAMS;
    EMCChannel::emcmotCommand.spindle = spindle;
    EMCChannel::emcmotCommand.maxLimit = max_pos;
    EMCChannel::emcmotCommand.minLimit = min_neg;
    EMCChannel::emcmotCommand.min_pos_speed = min_pos;
    EMCChannel::emcmotCommand.max_neg_speed = max_neg;
    EMCChannel::emcmotCommand.home = home_angle;
    EMCChannel::emcmotCommand.search_vel = search_vel;
    EMCChannel::emcmotCommand.home_sequence = sequence;
    EMCChannel::emcmotCommand.offset = increment;

    EMCChannel::push();
    return 0;
}
