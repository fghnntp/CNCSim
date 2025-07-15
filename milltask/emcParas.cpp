#include "emcParas.h"
#include "emc.hh"
#include "rs274ngc_interp.hh"	// the interpreter
#include "emcIniFile.hh"
#include "emcglb.h"
#include "interpl.hh"
#include <iostream>

extern void InitonUserMotionIF(void);
extern void InitTaskinft(void);
extern TrajConfig_t *GetTrajConfig();
extern JointConfig_t *GetJointConfig(int joint);
extern AxisConfig_t *GetAxisConfig(int axis);
extern SpindleConfig_t *GetSpindleConfig(int spindle);

void EMCParas::init_emc_paras()
{
    // This is motion simulation init and task simulation init
    emcStatus->motion.traj.axis_mask=~0;
    InitonUserMotionIF();
    InitTaskinft();
}

void EMCParas::set_traj_maxvel(double vel)
{
    GetTrajConfig()->MaxVel = vel;
}




