#pragma once
#include <cmath>
#include <array>

/*
 * Five-axis kinematics (header-only)
 * - Forward kinematics: (X,Y,Z, angle1, angle2) -> tip position & tool dir
 * - Inverse "linear-only": given (tip position, angle1, angle2) -> X,Y,Z
 * - No dependencies beyond <cmath>/<array>, no dynamic allocation
 *
 * 约定（与讨论保持一致）：
 * - 列向量 & 右乘变换；
 * - 变换链： T_total = Trans(XYZ) * T_base_to_primary * R1 * T12 * R2 * Tswing * Ttool
 * - T12 与 Tswing 中的平移，采用“各自轴的零位局部坐标”表达（预计算）
 * - 第二轴为“动轴”：其瞬时轴向 n2_eff = R1 * n2
 * - 刀长沿刀具局部 -Z（由 tool_axis_sign 与 tool_dir 决定零位刀轴方向，仅影响方向号，不影响 -Z 约定）
 */

namespace fiveaxis {

// ---------- Small math primitives ----------

struct Vec3 {
    double x=0, y=0, z=0;
    Vec3()=default;
    Vec3(double X,double Y,double Z):x(X),y(Y),z(Z){}
};
inline Vec3 operator+(const Vec3&a,const Vec3&b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec3 operator-(const Vec3&a,const Vec3&b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec3 operator*(double s,const Vec3&a){return {s*a.x,s*a.y,s*a.z};}
inline Vec3 operator*(const Vec3&a,double s){return s*a;}
inline double dot(const Vec3&a,const Vec3&b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Vec3 cross(const Vec3&a,const Vec3&b){
    return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}
inline double norm(const Vec3&a){return std::sqrt(dot(a,a));}
inline Vec3 unit(const Vec3&a){ double n=norm(a); return (n>0)? (1.0/n)*a : Vec3(0,0,0); }

struct Mat3 {
    // column-major for easy "R * v" with columns as basis
    // m[c*3 + r] = element (r,c)
    std::array<double,9> m{};
    static Mat3 I(){
        Mat3 R; R.m={1,0,0, 0,1,0, 0,0,1}; return R;
    }
};
inline Vec3 mul(const Mat3&R,const Vec3&v){
    // column-major
    return {
        R.m[0]*v.x + R.m[3]*v.y + R.m[6]*v.z,
        R.m[1]*v.x + R.m[4]*v.y + R.m[7]*v.z,
        R.m[2]*v.x + R.m[5]*v.y + R.m[8]*v.z
    };
}
inline Mat3 mul(const Mat3&A,const Mat3&B){
    Mat3 C;
    // C = A*B
    for(int c=0;c<3;++c){
        for(int r=0;r<3;++r){
            C.m[c*3+r] =
                A.m[0*3+r]*B.m[c*3+0] +
                A.m[1*3+r]*B.m[c*3+1] +
                A.m[2*3+r]*B.m[c*3+2];
        }
    }
    return C;
}

// Rodrigues rotation: axis is unit, angle in degrees
inline Mat3 rodrigues(const Vec3&axis_unit, double angle_deg){
    const double th = angle_deg * M_PI / 180.0;
    const double c = std::cos(th), s = std::sin(th);
    const double ux=axis_unit.x, uy=axis_unit.y, uz=axis_unit.z;

    // R = I*c + (1-c)uu^T + [u]_x s
    Mat3 R = Mat3::I();

    R.m[0] = c + (1-c)*ux*ux;
    R.m[4] = c + (1-c)*uy*uy;
    R.m[8] = c + (1-c)*uz*uz;

    R.m[3] = (1-c)*ux*uy - s*uz;
    R.m[6] = (1-c)*ux*uz + s*uy;

    R.m[1] = (1-c)*uy*ux + s*uz;
    R.m[7] = (1-c)*uy*uz - s*ux;

    R.m[2] = (1-c)*uz*ux - s*uy;
    R.m[5] = (1-c)*uz*uy + s*ux;

    return R;
}

// ---------- Kinematics model ----------

enum class MachineType { TABLE_TILTING=1, SPINDLE_TILTING=2, TABLE_SPINDLE_TILTING=3 };
enum class RotaryAxis { A, B, C };

struct Config {
    MachineType mtype = MachineType::TABLE_TILTING;

    // 两转轴的“名称顺序”（决定一轴二轴）
    RotaryAxis axis1 = RotaryAxis::A;
    RotaryAxis axis2 = RotaryAxis::B;

    // 每轴旋向（控制角 -> 物理角），通常 +1 或 -1
    int sign_axis1 = +1;
    int sign_axis2 = +1;

    // 轴向（世界系），允许提供 deflection（若全零则使用标准轴）
    Vec3 axis1_dir_world {1,0,0}; // A->X
    Vec3 axis2_dir_world {0,1,0}; // B->Y

    // 世界下测得的主轴心 & 二轴相对一轴的偏移
    Vec3 primary_center_world {0,0,0};         // T_base_to_primary
    Vec3 secondary_offset_world {0,0,0};       // measured in world

    // 非双转台机型：主轴摆动相对第二轴中心偏移（世界测得）
    Vec3 spindle_swing_world {0,0,0};          // for heads; zero if none

    // 工具方向（零位）：'X','Y','Z' & 号位（+1/-1）
    char tool_dir = 'Z';
    int  tool_axis_sign = +1;

    // 刀长（沿工具坐标 -Z）
    double tool_length = 0.0;
};

// 用户输入关节
struct JointValues {
    double X=0, Y=0, Z=0;  // 线性
    double angle1_deg=0;   // 与 axis1 对应的控制角
    double angle2_deg=0;   // 与 axis2 对应的控制角
};

struct Angles {
    double angle1_deg=0;
    double angle2_deg=0;
};

class Solver {
public:
    // 供外部读取的归一化轴向
    Vec3 n1_world{1,0,0}, n2_world{0,1,0};

    // 预计算：世界->各轴零位局部 的平移（在“各自零位局部坐标”下表示）
    Vec3 t12_local0{0,0,0};
    Vec3 swing_local0{0,0,0};

    // 原始配置（缓存）
    Config cfg;

    void configure(const Config& c){
        cfg = c;

        // 轴向：若提供为零向量则回退标准轴
        n1_world = unit(c.axis1_dir_world);
        if (norm(n1_world) == 0) n1_world = stdAxis(c.axis1);
        n2_world = unit(c.axis2_dir_world);
        if (norm(n2_world) == 0) n2_world = stdAxis(c.axis2);

        // 二轴偏移：把世界下的 offset 变换到“一轴零位局部坐标”
        // swing 偏移：把世界下的 offset 变换到“二轴零位局部坐标”
        Mat3 R_l1_to_w = localBasisZero(c.axis1, n1_world);
        Mat3 R_l2_to_w = localBasisZero(c.axis2, n2_world);
        // 局部 = R^T * 世界
        t12_local0 = mul(transpose(R_l1_to_w), c.secondary_offset_world);
        swing_local0 = mul(transpose(R_l2_to_w), c.spindle_swing_world);
    }

    // ---------- Forward kinematics ----------
    // 输入：X/Y/Z + 两个角；输出：刀尖位置 & 刀轴方向（世界）
    void forwardKinematics(const JointValues& q, Vec3& tip_pos_world, Vec3& tool_dir_world) const {
        // 1) 物理角（考虑旋向）
        const double th1 = cfg.sign_axis1 * q.angle1_deg;
        const double th2 = cfg.sign_axis2 * q.angle2_deg;

        // 2) R1 & n2_eff & R2
        const Mat3 R1 = rodrigues(n1_world, th1);
        const Vec3 n2_eff = unit(mul(R1, n2_world)); // 第二轴随动后的瞬时轴向
        const Mat3 R2 = rodrigues(n2_eff, th2);

        // 3) 累加旋转（仅用于求方向）
        const Mat3 R = ::fiveaxis::mul(R1, R2);

        // 4) 固定链平移（不含线性 XYZ）：按  Trans(a) & Rot(R) 组合规则，t += R_curr * t_local
        Vec3 t = cfg.primary_center_world; // T_base_to_primary
        // after R1:
        Vec3 t12_global = mul(R1, t12_local0);
        t = t + t12_global;
        // after R1*R2:
        Vec3 swing_global = mul(R, swing_local0);
        t = t + swing_global;
        // tool length along local -Z of tool frame: 先经 R 旋转
        Vec3 ttool_local0 {0,0,-cfg.tool_length};
        Vec3 ttool_global = mul(R, ttool_local0);
        t = t + ttool_global;

        // 5) 线性 XYZ 叠加
        Vec3 lin {q.X, q.Y, q.Z};
        tip_pos_world = lin + t;

        // 6) 刀轴方向
        tool_dir_world = toolAxisZero();
        tool_dir_world = mul(R, tool_dir_world);
        tool_dir_world = unit(tool_dir_world);
    }

    // ---------- Inverse (linear-only) ----------
    // 已知转角（控制角）与目标刀尖坐标，直接求 X/Y/Z
    void inverseLinearOnly(const Vec3& target_tip_world,
                           const Angles& ang,
                           Vec3& xyz_out) const
    {
        // 1) 物理角
        const double th1 = cfg.sign_axis1 * ang.angle1_deg;
        const double th2 = cfg.sign_axis2 * ang.angle2_deg;

        // 2) R1, n2_eff, R2, R
        const Mat3 R1 = rodrigues(n1_world, th1);
        const Vec3 n2_eff = unit(mul(R1, n2_world));
        const Mat3 R2 = rodrigues(n2_eff, th2);
        const Mat3 R = ::fiveaxis::mul(R1, R2);

        // 3) 同 forward：固定链平移 t_fixed
        Vec3 t = cfg.primary_center_world;
        t = t + mul(R1, t12_local0);
        t = t + mul(R, swing_local0);
        t = t + mul(R, Vec3{0,0,-cfg.tool_length});

        // 4) XYZ = target - t_fixed
        xyz_out = target_tip_world - t;
    }

private:
    // 工具零位方向（含 sign）
    Vec3 toolAxisZero() const {
        Vec3 e;
        switch (cfg.tool_dir) {
        case 'X': e = {1,0,0}; break;
        case 'Y': e = {0,1,0}; break;
        default : e = {0,0,1}; break;
        }
        if (cfg.tool_axis_sign < 0) e = -1.0 * e;
        return e;
    }

    // 标准轴向（世界）
    static Vec3 stdAxis(RotaryAxis a){
        if (a==RotaryAxis::A) return {1,0,0};   // X
        if (a==RotaryAxis::B) return {0,1,0};   // Y
        return {0,0,1};                         // C->Z
    }

    // 构造“一轴/二轴的零位局部到世界”的旋转矩阵：
    // A 轴：局部 x 对齐轴向；B 轴：局部 y；C 轴：局部 z。
    static Mat3 localBasisZero(RotaryAxis name, const Vec3& axis_world_unit){
        const Vec3 u = unit(axis_world_unit);
        // 选择一个 up 提示，避免与 u 共线
        Vec3 up = (std::fabs(u.z) < 0.9) ? Vec3{0,0,1} : Vec3{1,0,0};
        // 构造与 u 正交的向量
        Vec3 a = up - dot(up,u)*u;
        if (norm(a) < 1e-12) a = Vec3{1,0,0} - dot(Vec3{1,0,0},u)*u;
        a = unit(a);

        Vec3 ex, ey, ez;
        if (name==RotaryAxis::A){
            ex = u;
            ey = a;
            ez = cross(ex,ey);
        } else if (name==RotaryAxis::B){
            ey = u;
            ex = a;
            ez = cross(ex,ey);
        } else { // C
            ez = u;
            ex = a;
            ey = cross(ez,ex);
        }
        // 列为局部基在世界下的表达
        Mat3 R;
        R.m = {ex.x, ex.y, ex.z,
               ey.x, ey.y, ey.z,
               ez.x, ez.y, ez.z};
        return R;
    }

    static Mat3 transpose(const Mat3&R){
        Mat3 T;
        T.m = { R.m[0], R.m[3], R.m[6],
               R.m[1], R.m[4], R.m[7],
               R.m[2], R.m[5], R.m[8] };
        return T;
    }

    static Vec3 mul(const Mat3&R, const Vec3&local){ return ::fiveaxis::mul(R,local); }
};

} // namespace fiveaxis
