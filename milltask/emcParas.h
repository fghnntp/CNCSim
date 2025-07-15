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

    static void init_emc_paras();
    static void set_traj_maxvel(double vel);

    EMCParas() = delete;

};

#endif
