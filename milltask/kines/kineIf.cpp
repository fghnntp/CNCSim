#include "kineIf.h"
#include "switchkins.h"
#include "emcmotcfg.h"
#include <iostream>
#include <math.h>

Kines &Kines::GetInstance()
{
    static Kines _instance;
    return _instance;
}

void Kines::SetKineType(int type)
{
    if (type > kKineTypeNone && type < kKineTypeNum)
        kinType_ = type;
}

int Kines::GetKineType()
{
    return kinType_;
}

bool Kines::init()
{
    reset();
}

bool Kines::reset()
{
    kinType_ = kKineTypeIDENTITY;
}

int
Kines::KinematicsForward(const double *joint, EmcPose *pos,
                         const KINEMATICS_FORWARD_FLAGS *fflags,
                         KINEMATICS_INVERSE_FLAGS *iflags)
{
    switch (kinType_) {
    case kKineTypeIDENTITY:
        ForwardIdentity(joint, pos, fflags, iflags);
        break;
    case kKineTypeXYZABTRT:
        ForwardXYZABTRT(joint, pos, fflags, iflags);
        break;
    case kKineTypeXYZACTRT:
        ForwardXYZACTRT(joint, pos, fflags, iflags);
        break;
    defualt:
        ForwardIdentity(joint, pos, fflags, iflags);
        break;
    }
}

int
Kines::KinematicsInverse(const EmcPose *pos, double *joint,
                         const KINEMATICS_INVERSE_FLAGS *iflags,
                         KINEMATICS_FORWARD_FLAGS *fflags)
{
    switch (kinType_) {
    case kKineTypeIDENTITY:
        InverseIDentity(pos, joint, iflags, fflags);
        break;
    case kKineTypeXYZABTRT:
        InverseXYZABTRT(pos, joint, iflags, fflags);
        break;
    case kKineTypeXYZACTRT:
        InverseXYZACTRT(pos, joint, iflags, fflags);
    defualt:
        InverseIDentity(pos, joint, iflags, fflags);
        break;
    }
}

int
Kines::ForwardIdentity(const double *j, EmcPose *pos,
                       const KINEMATICS_FORWARD_FLAGS *fflags,
                       KINEMATICS_INVERSE_FLAGS *iflags)
{
    (void)fflags;
    (void)iflags;

    pos->tran.x = j[0];
    pos->tran.y = j[1];
    pos->tran.z = j[2];
    pos->a      = j[3];
    pos->b      = j[4];

    // unused coordinates:
    pos->c = 0;
    pos->u = 0;
    pos->v = 0;
    pos->w = 0;

    return 0;
}

int
Kines::InverseIDentity(const EmcPose *pos, double *j,
                       const KINEMATICS_INVERSE_FLAGS *iflags,
                       KINEMATICS_FORWARD_FLAGS *fflags)
{
    (void)iflags;
    (void)fflags;

    j[0] = pos->tran.x;
    j[1] = pos->tran.y;
    j[2] = pos->tran.z;
    j[3] = pos->a;
    j[4] = pos->b;
}


int
Kines::ForwardXYZABTRT(const double *j, EmcPose *pos,
                       const KINEMATICS_FORWARD_FLAGS *fflags,
                       KINEMATICS_INVERSE_FLAGS *iflags)
{
    (void)fflags;
    (void)iflags;
    double x_rot_point = x_rot_point_;
    double y_rot_point = y_rot_point_;
    double z_rot_point = z_rot_point_;

    double dz = z_offset_;
    double dt = tool_offset_z_;

    // substitutions as used in mathematical documentation
    // including degree -> radians angle conversion
    double       sa = sin(j[3]*TO_RAD);
    double       ca = cos(j[3]*TO_RAD);
    double       sb = sin(j[4]*TO_RAD);
    double       cb = cos(j[4]*TO_RAD);

    // used to be consistent with math in the documentation
    double       px = 0;
    double       py = 0;
    double       pz = 0;

    px          = j[0] - x_rot_point;
    py          = j[1] - y_rot_point;
    pz          = j[2] - z_rot_point - dt;

    pos->tran.x =   cb*px + sb*pz
                  + x_rot_point;

    pos->tran.y =   sa*sb*px + ca*py - cb*sa*pz + sa*dz
                  + y_rot_point;

    pos->tran.z = - ca*sb*px + sa*py + ca*cb*pz - ca*dz
                  + z_rot_point + dz + dt;

    pos->a      = j[3];
    pos->b      = j[4];
    pos->c      = j[5];

    // unused coordinates:
    pos->c = 0;
    pos->u = 0;
    pos->v = 0;
    pos->w = 0;

    return 0;
}

int
Kines::InverseXYZABTRT(const EmcPose * pos,
                       double *j,
                       const KINEMATICS_INVERSE_FLAGS * iflags,
                       KINEMATICS_FORWARD_FLAGS * fflags)
{
    (void)iflags;
    (void)fflags;
    double x_rot_point = x_rot_point_;
    double y_rot_point = y_rot_point_;
    double z_rot_point = z_rot_point_;

    double dx = x_offset_;
    double dz = z_offset_;
    double dt = tool_offset_z_;

    // substitutions as used in mathematical documentation
    // including degree -> radians angle conversion
    double       sa = sin(pos->a*TO_RAD);
    double       ca = cos(pos->a*TO_RAD);
    double       sb = sin(pos->b*TO_RAD);
    double       cb = cos(pos->b*TO_RAD);

    // used to be consistent with math in the documentation
    double       qx = 0;
    double       qy = 0;
    double       qz = 0;

    qx   = pos->tran.x - x_rot_point - dx;
    qy   = pos->tran.y - y_rot_point;
    qz   = pos->tran.z - z_rot_point - dz - dt;

    j[0] =   cb*qx + sa*sb*qy - ca*sb*qz + cb*dx - sb*dz
           + x_rot_point;

    j[1] =   ca*qy + sa*qz
           + y_rot_point;

    j[2] =   sb*qx - sa*cb*qy + ca*cb*qz + sb*dx + cb*dz
           + z_rot_point + dt;

    j[3] = pos->a;
    j[4] = pos->b;

    return 0;
}

int Kines::ForwardXYZACTRT(const double *j, EmcPose *pos, const KINEMATICS_FORWARD_FLAGS *fflags, KINEMATICS_INVERSE_FLAGS *iflags)
{
    (void)fflags;
    (void)iflags;
    double x_rot_point = x_rot_point_;
    double y_rot_point = y_rot_point_;
    double z_rot_point = z_rot_point;
    double          dt = tool_offset_;
    double          dy = y_offset_;
    double          dz = z_offset_;
    double       a_rad = j[3]*TO_RAD;
    double       c_rad = j[5]*TO_RAD;

    dz = dz + dt;

    pos->tran.x = + cos(c_rad)              * (j[0]      - x_rot_point)
                 + sin(c_rad) * cos(a_rad) * (j[1] - dy - y_rot_point)
                 + sin(c_rad) * sin(a_rad) * (j[2] - dz - z_rot_point)
                 + sin(c_rad) * dy
                 + x_rot_point;

    pos->tran.y = - sin(c_rad)              * (j[0]      - x_rot_point)
                 + cos(c_rad) * cos(a_rad) * (j[1] - dy - y_rot_point)
                 + cos(c_rad) * sin(a_rad) * (j[2] - dz - z_rot_point)
                 + cos(c_rad) * dy
                 + y_rot_point;

    pos->tran.z = + 0
                 - sin(a_rad) * (j[1] - dy - y_rot_point)
                 + cos(a_rad) * (j[2] - dz - z_rot_point)
                 + dz
                 + z_rot_point;

    pos->a = j[3];
    pos->c = j[5];

    // optional letters (specify with coordinates module parameter)
    pos->b =  j[4];
    pos->u =  j[6];
    pos->v =  j[7];
    pos->w =  j[8];

    return 0;
}

int Kines::InverseXYZACTRT(const EmcPose *pos, double *j, const KINEMATICS_INVERSE_FLAGS *iflags, KINEMATICS_FORWARD_FLAGS *fflags)
{
    (void)iflags;
    (void)fflags;
    double x_rot_point = x_rot_point_;
    double y_rot_point = y_rot_point_;
    double z_rot_point = z_rot_point_;
    double         dy  = y_offset_;
    double         dz  = z_offset_;
    double         dt  = tool_offset_;
    double      a_rad  = pos->a*TO_RAD;
    double      c_rad  = pos->c*TO_RAD;

    EmcPose P; // computed position

    dz = dz + dt;

    P.tran.x   = + cos(c_rad)              * (pos->tran.x - x_rot_point)
                - sin(c_rad)              * (pos->tran.y - y_rot_point)
                + x_rot_point;

    P.tran.y   = + sin(c_rad) * cos(a_rad) * (pos->tran.x - x_rot_point)
                + cos(c_rad) * cos(a_rad) * (pos->tran.y - y_rot_point)
                -              sin(a_rad) * (pos->tran.z - z_rot_point)
                -              cos(a_rad) * dy
                +              sin(a_rad) * dz
                + dy
                + y_rot_point;

    P.tran.z   = + sin(c_rad) * sin(a_rad) * (pos->tran.x - x_rot_point)
                + cos(c_rad) * sin(a_rad) * (pos->tran.y - y_rot_point)
                +              cos(a_rad) * (pos->tran.z - z_rot_point)
                -              sin(a_rad) * dy
                -              cos(a_rad) * dz
                + dz
                + z_rot_point;


    P.a        = pos->a;
    P.c        = pos->c;

    // optional letters (specify with coordinates module parameter)
    P.b = pos->b;
    P.u = pos->u;
    P.v = pos->v;
    P.w = pos->w;
}

int kinematicsForward(const double *joint,
                      EmcPose * pos,
                      const KINEMATICS_FORWARD_FLAGS * fflags,
                      KINEMATICS_INVERSE_FLAGS * iflags)
{
    Kines::GetInstance().KinematicsForward(joint,
                                           pos,
                                           fflags,
                                           iflags);
    return 0;
}

int kinematicsInverse(const EmcPose * pos,
                      double *joint,
                      const KINEMATICS_INVERSE_FLAGS * iflags,
                      KINEMATICS_FORWARD_FLAGS * fflags)
{
    Kines::GetInstance().KinematicsInverse(pos,
                                           joint,
                                           iflags,
                                           fflags);
    return 0;
}

KINEMATICS_TYPE kinematicsType()
{
    return KINEMATICS_BOTH;
}

int kinematicsSwitchable() {return 1;}

int kinematicsSwitch(int new_switchkins_type)
{
    switch (new_switchkins_type) {
    case Kines::kKineTypeIDENTITY:
        Kines::GetInstance().SetKineType(Kines::kKineTypeIDENTITY);
        break;
    case Kines::kKineTypeXYZABTRT:
        Kines::GetInstance().SetKineType(Kines::kKineTypeXYZABTRT);
        break;
    case Kines::kKineTypeXYZACTRT:
        Kines::GetInstance().SetKineType(Kines::kKineTypeXYZACTRT);
        break;
    defaul:
        Kines::GetInstance().SetKineType(Kines::kKineTypeIDENTITY);
        break;
    }

    return 0;
}
