#pragma once

#include "kinematics.h"
#include <string>

class Kines {
public:
    enum KineSwitchableType {
        kKineTypeNone,
        kKineTypeIDENTITY,
        kKineTypeXYZABTRT,
        kKineTypeXYZACTRT,
        kKineTypeNum,
    };

    Kines(const Kines&) = delete;
    Kines& operator=(const Kines&) = delete;

    static Kines& GetInstance();
    void SetKineType(int type);
    int GetKineType(void);
    bool init(void);
    bool reset(void);

    int KinematicsForward(const double *joint,
                          EmcPose * pos,
                          const KINEMATICS_FORWARD_FLAGS * fflags,
                          KINEMATICS_INVERSE_FLAGS * iflags);

    int KinematicsInverse(const EmcPose * pos,
                          double *joint,
                          const KINEMATICS_INVERSE_FLAGS * iflags,
                          KINEMATICS_FORWARD_FLAGS * fflags);

    int ForwardIdentity(const double *j,
                        EmcPose * pos,
                        const KINEMATICS_FORWARD_FLAGS * fflags,
                        KINEMATICS_INVERSE_FLAGS * iflags);

    int InverseIDentity(const EmcPose * pos,
                          double *j,
                          const KINEMATICS_INVERSE_FLAGS * iflags,
                          KINEMATICS_FORWARD_FLAGS * fflags);

    int ForwardXYZABTRT(const double *j,
                        EmcPose * pos,
                        const KINEMATICS_FORWARD_FLAGS * fflags,
                        KINEMATICS_INVERSE_FLAGS * iflags);

    int InverseXYZABTRT(const EmcPose * pos,
                        double *j,
                        const KINEMATICS_INVERSE_FLAGS * iflags,
                        KINEMATICS_FORWARD_FLAGS * fflags);

    int ForwardXYZACTRT(const double *j,
                        EmcPose * pos,
                        const KINEMATICS_FORWARD_FLAGS * fflags,
                        KINEMATICS_INVERSE_FLAGS * iflags);

    int InverseXYZACTRT(const EmcPose * pos,
                        double *j,
                        const KINEMATICS_INVERSE_FLAGS * iflags,
                        KINEMATICS_FORWARD_FLAGS * fflags);




private:
    Kines() = default;

    int kinType_ = kKineTypeIDENTITY;
    double tool_offset_z_ = 10.0;
    double tool_offset_ = 10.0;
    double x_offset_ = 10.0;
    double y_offset_ = 10.0;
    double z_offset_ = 20.0;
    double x_rot_point_ = 15.0;
    double y_rot_point_ = 20.0;
    double z_rot_point_ = 25.0;

};
