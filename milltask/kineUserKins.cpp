#include "kinematics.h"

// support for template for user-defined switchkins_type==2
int userkKinematicsSetup(const int   comp_id,
                        const char* coordinates,
                        kparms*     ksetup_parms)
{
    return 0;
}

int userkKinematicsForward(const double *joint,
                          struct EmcPose * world,
                          const KINEMATICS_FORWARD_FLAGS * fflags,
                          KINEMATICS_INVERSE_FLAGS * iflags)
{
    return 0;
}

int userkKinematicsInverse(const struct EmcPose * world,
                          double *joint,
                          const KINEMATICS_INVERSE_FLAGS * iflags,
                          KINEMATICS_FORWARD_FLAGS * fflags)
{
    return 0;
}
