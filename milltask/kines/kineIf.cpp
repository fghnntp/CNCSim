#include "kineIf.h"
#include "switchkins.h"
#include "emcmotcfg.h"
#include <iostream>
#include <math.h>

using namespace fiveaxis;
Kines::Kines()
{
    // 1) 配置（示例：一台混合型 AC，主轴摆头）
    cfg_.mtype = MachineType::TABLE_SPINDLE_TILTING;
    cfg_.axis1 = RotaryAxis::A;            // 第一轴 A
    cfg_.axis2 = RotaryAxis::B;            // 第二轴 C
    cfg_.sign_axis1 = +1;                  // 控制角与物理角同号
    cfg_.sign_axis2 = +1;
    cfg_.axis1_dir_world = {1,0,0};        // A 沿 X
    cfg_.axis2_dir_world = {0,0,1};        // C 沿 Z
    cfg_.primary_center_world = {455,0,650};
    cfg_.secondary_offset_world = {-455,250,-850};
    cfg_.spindle_swing_world   = {0,-250,200}; // 摆头偏置（头架）
    cfg_.tool_dir = 'Z';
    cfg_.tool_axis_sign = +1;
    cfg_.tool_length = 0.0;

    solver_.configure(cfg_);

    // 2) 正解
    JointValues q;
    q.X = 100; q.Y = -50; q.Z = 300;
    q.angle1_deg = 20; q.angle2_deg = -35;

    fiveaxis::Vec3 pos, dir;
    solver_.forwardKinematics(q, pos, dir);
    std::printf("FK: P=(%.3f, %.3f, %.3f), n=(%.6f, %.6f, %.6f)\n",
                pos.x,pos.y,pos.z, dir.x,dir.y,dir.z);

    // 3) 线性逆解（角度已知）
    fiveaxis::Angles ang{q.angle1_deg, q.angle2_deg};
    fiveaxis::Vec3 xyz;
    solver_.inverseLinearOnly(pos, ang, xyz);
    std::printf("IK-linear: (X,Y,Z)=(%.3f, %.3f, %.3f)\n", xyz.x,xyz.y,xyz.z);

}

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

void Kines::init()
{
    reset();
}

void Kines::reset()
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
    case kKineTypeFiveAxis:
        ForwardFiveAxis(joint, pos, fflags, iflags);
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
    return 0;
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
    case kKineTypeFiveAxis:
        InverseFiveAxis(pos, joint, iflags, fflags);
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
    return 0;
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

    return 0;
}


int
Kines::ForwardXYZABTRT(const double *j, EmcPose *pos,
                       const KINEMATICS_FORWARD_FLAGS *fflags,
                       KINEMATICS_INVERSE_FLAGS *iflags)
{
    (void)fflags;
    (void)iflags;
    double t_x = x_rot_point_;
    double t_y = y_rot_point_;
    double t_z = z_rot_point_;

    double t_x1 = x_offset_;
    double t_y1 = y_offset_;
    double t_z1 = z_offset_;
    double tool_length = tool_offset_z_;

    double det_x = 0.0, det_y = 0.0, det_z = 0.0;

    // substitutions as used in mathematical documentation
    // including degree -> radians angle conversion
    double       sa = sin(j[3]*TO_RAD);
    double       ca = cos(j[3]*TO_RAD);
    double       sb = sin(j[4]*TO_RAD);
    double       cb = cos(j[4]*TO_RAD);

   // 从 T_POS * [0,0,0,1] 开始
   double x = j[0];
   double y = j[1];
   double z = j[2];

   // T_TS2Tool(0,0,-L)
   z -= tool_length;

   // T_B2RB(-t_x,-t_y,-t_z) : 等价 x-=t_x, y-=t_y, z-=t_z
   x -= t_x;
   y -= t_y;
   z -= t_z;

    // R_B (绕Y： [cb*x + sb*z, y, -sb*x + cb*z])
    {
        const double x2 =  cb*x + sb*z;
        const double y2 =  y;
        const double z2 = -sb*x + cb*z;
        x = x2; y = y2; z = z2;
    }

    // T_RB2RA(-t_x1,-t_y1,-t_z1)
    x -= t_x1;
    y -= t_y1;
    z -= t_z1;

    // R_A (绕X： [x, ca*y - sa*z, sa*y + ca*z])
    {
       const double x2 = x;
       const double y2 = ca*y - sa*z;
       const double z2 = sa*y + ca*z;
       x = x2; y = y2; z = z2;
    }

    // W_OFFSET (det_x, det_y, det_z)
    x += det_x;
    y += det_y;
    z += det_z;

    pos->tran.x = x;
    pos->tran.y = y;
    pos->tran.z = z;

    pos->a      = j[3];
    pos->b      = j[4];
    pos->c      = j[5];

    //uinit vector_z
    tad_[0] = sb*1.0;

    tad_[1] = - sa*cb*1.0;

    tad_[2] = ca*cb*1.0;


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
    double t_x = x_rot_point_;
    double t_y = y_rot_point_;
    double t_z = z_rot_point_;

    double t_x1 = x_offset_;
    double t_y1 = y_offset_;
    double t_z1 = z_offset_;
    double tool_length = tool_offset_z_;

    double det_x = 0.0, det_y = 0.0, det_z = 0.0;

    // substitutions as used in mathematical documentation
    // including degree -> radians angle conversion
    double       sa = sin(pos->a*TO_RAD);
    double       ca = cos(pos->a*TO_RAD);
    double       sb = sin(pos->b*TO_RAD);
    double       cb = cos(pos->b*TO_RAD);

    // 从 [Q]-det 开始（W_OFFSET^{-1}）
    double x = pos->tran.x - det_x;
    double y = pos->tran.y - det_y;
    double z = pos->tran.z - det_z;

    // R_A^{-1} (绕X -a)：[x, ca*y + sa*z, -sa*y + ca*z]
    {
        const double x1 = x;
        const double y1 =  ca*y + sa*z;
        const double z1 = -sa*y + ca*z;
        x = x1; y = y1; z = z1;
    }

    // T_RB2RA^{-1} (+t1)
    x += t_x1;
    y += t_y1;
    z += t_z1;

    // R_B^{-1} (绕Y -b)：[cb*x - sb*z, y, sb*x + cb*z]
    {
        const double x1 =  cb*x - sb*z;
        const double y1 =  y;
        const double z1 =  sb*x + cb*z;
        x = x1; y = y1; z = z1;
    }

    // T_B2RB^{-1} (+t)
    x += t_x;
    y += t_y;
    z += t_z;

    // T_TS2Tool^{-1} (0,0,+L)
    z += tool_length;


    // + (t_x, t_y, t_z) , + (0,0,tool_length), -pos
    j[0] = x;
    j[1] = y;
    j[2] = z;

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

    return 0;
}

int Kines::ForwardFiveAxis(const double *j, EmcPose *pos, const KINEMATICS_FORWARD_FLAGS *fflags, KINEMATICS_INVERSE_FLAGS *iflags)
{
    (void)iflags;
    (void)fflags;

    JointValues q;
    q.X = j[0];
    q.Y = j[1];
    q.Z = j[2];
    q.angle1_deg = j[3];
    q.angle2_deg = j[4];

    fiveaxis::Vec3 toolPos, dir;

    solver_.forwardKinematics(q, toolPos, dir);

    pos->tran.x = q.X;
    pos->tran.y = q.Y;
    pos->tran.z = q.Z;
    pos->a = j[3];
    pos->b = j[4];
    pos->c = j[5];
    pos->u = j[6];
    pos->v = j[7];
    pos->w = j[8];

    return 0;
}

int Kines::InverseFiveAxis(const EmcPose *pos, double *j, const KINEMATICS_INVERSE_FLAGS *iflags, KINEMATICS_FORWARD_FLAGS *fflags)
{
    (void)iflags;
    (void)fflags;
    JointValues q;
    fiveaxis::Vec3 toolPos, dir;

    toolPos.x = pos->tran.x;
    toolPos.y = pos->tran.y;
    toolPos.z = pos->tran.z;


    fiveaxis::Angles ang{pos->a, pos->b};
    fiveaxis::Vec3 xyz;
    solver_.inverseLinearOnly(toolPos, ang, xyz);

    j[0] = xyz.x;
    j[1] = xyz.y;
    j[2] = xyz.z;
    j[3] = pos->a;
    j[4] = pos->b;
    j[5] = pos->c;
    j[6] = pos->u;
    j[7] = pos->v;
    j[8] = pos->w;

    return 0;
}

std::array<double, 3> Kines::GetTad()
{
    return tad_;
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
    case Kines::kKineTypeFiveAxis:
        Kines::GetInstance().SetKineType(Kines::kKineTypeFiveAxis);
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
