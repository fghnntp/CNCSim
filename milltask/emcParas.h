#ifndef _EMC_PARAS_
#define _EMC_PARAS_

#include "emc.hh"
#include "rs274ngc_interp.hh"	// the interpreter
#include "emcIniFile.hh"
#include "emcglb.h"
#include "interpl.hh"

class EMCParas {
    //Paras include milltask and emc area
    //These interface also control the rs274 connon
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


    static TrajConfig_t *GetTrajConfig();
    static JointConfig_t *GetJointConfig(int joint);
    static AxisConfig_t *GetAxisConfig(int axis);
    static SpindleConfig_t* GetSpindleConfig(int spindle);
    static void InitTaskinft(void);


    static std::string inifileName;
    static void init_emc_paras();
    static void set_traj_maxvel(double vel);

    static MotTrajConfig trajconfig;
    static MotJointConfig jointconfig[EMCMOT_MAX_JOINTS];
    static MotAxisConfig axisconfig[EMCMOT_MAX_AXIS];

    static int iniTraj(const char *filename);
    static int loadTraj(EmcIniFile *trajInifile);
    static int loadKins(EmcIniFile *trajInifile);

    static std::string showTrajParas();

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

    static int iniJoint(int joint, const char *filename);
    static int loadJoint(int joint, EmcIniFile *jointIniFile);

    static std::string showJoint(int joint = 0);

    static int emcJointSetType_(int joint, unsigned char jointType);
    static int emcJointSetUnits_(int joint, double units);
    static int emcJointSetBacklash_(int joint, double backlash);
    static int emcJointSetMinPositionLimit_(int joint, double limit);
    static int emcJointSetMaxPositionLimit_(int joint, double limit);
    static int emcJointSetFerror_(int joint, double ferror);
    static int emcJointSetMinFerror_(int joint, double ferror);
    static int emcJointSetHomingParams_(int joint, double home, double offset, double home_final_vel,
                   double search_vel, double latch_vel,
                   int use_index, int encoder_does_not_reset,
                   int ignore_limits, int is_shared,
                   int sequence,int volatile_home, int locking_indexer,int absolute_encoder);
    static int emcJointSetMaxVelocity_(int joint, double vel);
    static int emcJointSetMaxAcceleration_(int joint, double acc);
    static int emcJointActivate_(int joint);

    static int iniAxis(int axis, const char *filename);
    static int loadAxis(int axis, EmcIniFile *axisIniFile);

    static std::string showAxis(int axis = 0);

    static int emcAxisSetMinPositionLimit_(int axis, double limit);
    static int emcAxisSetMaxPositionLimit_(int axis, double limit);
    static int emcAxisSetMaxVelocity_(int axis, double vel,double ext_offset_vel);
    static int emcAxisSetMaxAcceleration_(int axis, double acc,double ext_offset_acc);
    static int emcAxisSetLockingJoint_(int axis, int joint);

    static int iniSpindle(int spindle, const char *filename);
    static int loadSpindle(int spindle, EmcIniFile *spindleIniFile);

    static std::string showSpindle(int spindle = 0);

    static int emcSpindleSetParams_(int spindle, double max_pos, double min_pos, double max_neg,
                   double min_neg, double search_vel, double home_angle, int sequence, double increment);

    EMCParas() = delete;
};

#endif
