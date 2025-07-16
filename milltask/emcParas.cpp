#include "emcParas.h"
#include "emc.hh"
//#include "rs274ngc_interp.hh"	// the interpreter
#include "emcIniFile.hh"
#include "emcglb.h"
//#include "interpl.hh"
#include <iostream>
#include "emcIniFile.hh"
#include "motion.h"
#include "emcChannel.h"

extern void InitonUserMotionIF(void);
extern void InitTaskinft(void);
extern TrajConfig_t *GetTrajConfig();
extern JointConfig_t *GetJointConfig(int joint);
extern AxisConfig_t *GetAxisConfig(int axis);
extern SpindleConfig_t *GetSpindleConfig(int spindle);

std::string EMCParas::inifileName;
EMCParas::MotTrajConfig EMCParas::trajconfig;
EMCParas::MotJointConfig EMCParas::jointconfig;
EMCParas::MotAxisConfig EMCParas::axisconfig;

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
