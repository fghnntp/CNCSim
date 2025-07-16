#ifndef _EMC_PARAS_
#define _EMC_PARAS_

#include "emc.hh"
#include "rs274ngc_interp.hh"	// the interpreter
#include "emcIniFile.hh"
#include "emcglb.h"
#include "interpl.hh"

class EMCParas {
    //Paras include milltask and emc area
public:
    struct MotTrajConfig {
        double traj_default_velocity;
        double traj_max_velocity;
        double traj_default_acceleration;
        double traj_max_acceleration;

        bool traj_arc_blend_enable;
        bool traj_arc_blend_fallback_enable;
        int traj_arc_blend_optimization_depth;
        double traj_arc_blend_gap_cycles;
        double traj_arc_blend_ramp_freq;
        double traj_arc_blend_tangent_kink_ratio;


    };

    struct MotJointConfig {
        double joint_backlash;
        double joint_ferror;
        double joint_min_ferror;
        double joint_min_limit;
        double joint_max_limit;
        double joint_max_velocity;
        double joint_max_acceleration;
        double joint_home;
        double joint_home_offset;
        int joint_home_sequence;
    };

    struct MotAxisConfig {
        double axis_min_limit;
        double axis_max_limit;
        double axis_max_velocity;
        double axis_max_acceleration;
    };



    static std::string inifileName;
    static void init_emc_paras();
    static void set_traj_maxvel(double vel);

    static MotTrajConfig trajconfig;
    static MotJointConfig jointconfig;
    static MotAxisConfig axisconfig;

    static int iniTraj(const char *filename);
    static int loadTraj(EmcIniFile *trajInifile);
    static int loadKins(EmcIniFile *trajInifile);

    static int emcTrajSetAxes_(int axismask);
    static int emcTrajSetJoints_(int joints);
    static int emcTrajSetUnits_(double linearUnits, double angularUnits);
    static int emcTrajSetVelocity_(double vel, double ini_maxvel);
    static int emcTrajSetSpindles_(int spindles);
    static int emcTrajSetMaxVelocity_(double vel);
    static int emcTrajSetAcceleration_(double acc);
    static int emcTrajSetMaxAcceleration_(double acc);
    static int emcSetupArcBlends_(int arcBlendEnable,
                            int arcBlendFallbackEnable,
                            int arcBlendOptDepth,
                            int arcBlendGapCycles,
                            double arcBlendRampFreq,
                            double arcBlendTangentKinkRatio);
    static int emcSetMaxFeedOverride_(double maxFeedScale);
    static int emcSetProbeErrorInhibit_(int j_inhibit, int h_inhibit);
    static int emcTrajSetHome(const EmcPose& home);

    EMCParas() = delete;

};

#endif
