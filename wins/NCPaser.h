#pragma once
#include <QtCore>
#include <cmath>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <QtMath>

namespace NC {

//Basic NC Parts
enum class Unit { MM, INCH };
enum class DistanceMode { ABSOLUTE, INCREMENTAL };
enum class Plane { XY, ZX, YZ };
enum class MoveType { Rapid, Linear, ArcCW, ArcCCW };

struct Move {
    MoveType type{};
    QVector3D start{0,0,0};
    QVector3D end{0,0,0};
    double feed = 0.0;             // G0 时通常忽略
    Plane plane = Plane::XY;

    // 圆弧参数（任选其一）
    bool usesRadius = false;       // true=使用 R 半径；false=使用 center
    QVector3D center{0,0,0};       // 圆心（绝对坐标）
    double radius = 0.0;           // 半径（当 usesRadius=true 时有效）

    // 工艺参数
    int toolNum = 0;
    long long lineNum = -1;
    double time{0.0};
    double totalTime{0.0};
};

// 线性插值小工具
inline float lerp(float a, float b, float t){ return a + (b - a) * t; }

// 根据移动类型返回颜色；step∈[0,1] 仅对 Linear（G01）生效
inline QVector4D colorFor(MoveType t, float step = -1.0f, float alpha = 1.0f) {
    switch (t) {
    case MoveType::Rapid:  // G00
        return {1.00f, 0.76f, 0.03f, alpha};  // #FFC107
    case MoveType::Linear: // G01：蓝→绿渐变
    {
        if (step < 0.0f) step = 1.0f;                     // 默认用终点色
        step = std::clamp(step, 0.0f, 1.0f);

        // 蓝 #03A9F4 → 绿 #4CAF50
        const QVector3D B(0.01f, 0.66f, 0.96f);
        const QVector3D G(0.30f, 0.69f, 0.31f);
        QVector3D rgb( lerp(G.x(), B.x(), step),
                      lerp(G.y(), B.y(), step),
                      lerp(G.z(), B.z(), step) );
        return { rgb.x(), rgb.y(), rgb.z(), alpha };
    }
    case MoveType::ArcCW:  // G02
        return {0.01f, 0.66f, 0.96f, alpha};  // #03A9F4
    case MoveType::ArcCCW: // G03
        return {0.91f, 0.12f, 0.39f, alpha};  // #E91E63
    }
    return {1,1,1,alpha};
}

inline float widthFor(MoveType t) {
    return (t == MoveType::Rapid) ? 2.0f : 3.0f; // 快速细一点
}

inline float patIdFor(MoveType t) {
    return (t == MoveType::Rapid) ? 1.0f : 0.0f; // G00 用虚线，其它实线
}


struct ParseOptions {
    bool ijkIncremental = true;    // IJK 是否相对起点（多数控制器如此）
    bool allowHugeRArc = false;    // R 大弧（>180°）选择规则：false=短弧，true=长弧
};

struct MachineState {
    Unit unit = Unit::MM;
    DistanceMode dist = DistanceMode::ABSOLUTE;
    Plane plane = Plane::XY;
    double feed = 0.0;
    QVector3D pos{0,0,0};
    int toolNum{0};
};

static inline double norm3(const QVector3D& v){ return std::sqrt(double(v.x())*v.x() + double(v.y())*v.y() + double(v.z())*v.z()); }

// 把 3D 点投影到指定平面，得到 2D 向量
static inline QPointF projectToPlane(const QVector3D& p, Plane pl){
    switch (pl) {
    case Plane::XY: return QPointF(p.x(), p.y());
    case Plane::ZX: return QPointF(p.x(), p.z());
    case Plane::YZ: return QPointF(p.y(), p.z());
    }
    return QPointF(p.x(), p.y());
}

// 两向量夹角（0..π）
static inline double angleBetween(const QPointF& a, const QPointF& b){
    const double na = std::hypot(a.x(), a.y());
    const double nb = std::hypot(b.x(), b.y());
    if (na == 0 || nb == 0) return 0.0;
    double c = (a.x()*b.x() + a.y()*b.y()) / (na*nb);
    c = std::clamp(c, -1.0, 1.0);
    return std::acos(c);
}

// 2D 向量“叉乘 z 分量”，用于判断 CW/CCW 方向（>0 表示 a->b 逆时针）
static inline double crossZ(const QPointF& a, const QPointF& b){
    return a.x()*b.y() - a.y()*b.x();
}


static inline double linearLength(const Move& m){
    return norm3(m.end - m.start);
}

// 基于 center 的圆弧弧长
static inline double arcLength_center(const Move& m){
    // 投影到工作平面
    QPointF c  = projectToPlane(m.center, m.plane);
    QPointF s  = projectToPlane(m.start,  m.plane) - c;
    QPointF e  = projectToPlane(m.end,    m.plane) - c;

    // 半径取起点到圆心（更稳）
    double r = std::hypot(s.x(), s.y());
    if (r == 0.0) return 0.0;

    // 基本夹角
    double theta = angleBetween(s, e); // 0..π
    // 方向判断并扩展到 0..2π
    const bool ccw = (m.type == MoveType::ArcCCW);
    double z = crossZ(s, e);           // >0 表示 s->e 逆时针

    if (ccw) {
        if (z < 0) theta = 2*M_PI - theta; // 需要走到 e 的逆时针角
    } else { // CW
        if (z > 0) theta = 2*M_PI - theta; // 需要走到 e 的顺时针角
    }
    return r * theta;
}

// 基于半径 R 的圆弧弧长（圆心未知）：默认走“小圆弧”或“大圆弧”
static inline double arcLength_radius(const Move& m){
    // chord = 弦长（投影到圆弧所在平面）
    QPointF s = projectToPlane(m.start, m.plane);
    QPointF e = projectToPlane(m.end,   m.plane);
    double chord = std::hypot((e.x()-s.x()), (e.y()-s.y()));
    double R = std::abs(m.radius);
    if (R <= 0.0) return 0.0;

    // 两点与半径能构成圆的条件
    double ratio = std::clamp(chord / (2.0*R), -1.0, 1.0);
    double theta = 2.0 * std::asin(ratio);      // 0..π 的小圆弧角
    // if (!mp.arc_minor_default) theta = 2*M_PI - theta; // 若默认取大圆弧

    return R * theta;
}


// 返回“秒”；若 feed==0 保护为 0
static inline double timeSecondsForMove(const Move& m){
    constexpr double SEC_PER_MIN = 60.0;

    switch (m.type) {
    case MoveType::Rapid: {
        double L = linearLength(m);
        if (m.feed <= 1e-9) return 0.0;
        return SEC_PER_MIN * (L / m.feed);
    }
    case MoveType::Linear: {
        double L = linearLength(m);
        if (m.feed <= 1e-9) return 0.0;
        return SEC_PER_MIN * (L / m.feed);
    }
    case MoveType::ArcCW:
    case MoveType::ArcCCW: {
        double arcL = 0.0;
        if (!m.usesRadius) arcL = arcLength_center(m);
        else               arcL = arcLength_radius(m);
        if (m.feed <= 1e-9) return 0.0;
        return SEC_PER_MIN * (arcL / m.feed);
    }
    default:
        return 0.0;
    }
}

// // 批量计算：填充每段 time 与累计 totalTime（秒）
// static inline void computeTimes(std::vector<Move>& path){
//     double acc = 0.0;
//     for (auto& mv : path) {
//         mv.time = timeSecondsForMove(mv);
//         acc += mv.time;
//         mv.totalTime = acc;
//     }
// }






//Baisc NCPaser
class Parser {
public:
    explicit Parser(ParseOptions opt = {}) : opt_(opt) {}

    // 解析整段 NC 文本
    QVector<Move> parse(const QString& text) {
        long long lineNum = 0;
        moves_.clear();
        state_ = MachineState{};
        auto lines = text.split('\n');
        for (const QString& rawLine : lines) {
            const QString line = stripComments(rawLine).trimmed();
            lineNum++;
            if (line.isEmpty()) continue;
            parseLine(line, lineNum);
        }
        return moves_;
    }

    // 把一条移动指令离散成点（包含起点与终点）
    static QVector<QVector3D> toPolyline(const Move& mv,
                                         double maxSegLen = 1.0,
                                         double maxChordErr = 0.02) {
        QVector<QVector3D> pts;
        pts.reserve(32);
        pts.push_back(mv.start);
        switch (mv.type) {
        case MoveType::Rapid:
        case MoveType::Linear: {
            pts.push_back(mv.end);
            break;
        }
        case MoveType::ArcCW:
        case MoveType::ArcCCW: {
            // 在所在平面内离散
            auto info = arcPlanarInfo(mv);
            const double R = info.R;
            if (R <= 0 || !std::isfinite(R)) {
                pts.push_back(mv.end);
                break;
            }
            // 计算总夹角与方向
            double dtheta = signedAngle(info.startAngle, info.endAngle, mv.type == MoveType::ArcCCW);
            dtheta = normalizeAngleSigned(dtheta); // [-2π, 2π]合理化

            // 每段角度步长：由弦误差和最大段长共同限制
            double step1 = 2.0 * std::acos(std::max(0.0, 1.0 - maxChordErr / R));
            if (step1 <= 0 || std::isnan(step1)) step1 = M_PI / 90.0; // 2°
            double step2 = maxSegLen / std::max(1e-9, R);
            double step = std::min(step1, step2);
            step = std::clamp(step, M_PI/720.0, M_PI/6.0); // [0.25°, 30°] 约束

            int n = std::max(1, int(std::ceil(std::fabs(dtheta) / step)));
            const double sgn = (dtheta >= 0 ? 1.0 : -1.0);
            for (int i = 1; i < n; ++i) {
                double ang = info.startAngle + sgn * (std::fabs(dtheta) * (double(i) / n));
                pts.push_back(pointOnCircle(info.center, info.u, info.v, R, ang));
            }
            pts.push_back(mv.end);
            break;
        }
        }
        return pts;
    }
protected:
    void parseLine(const QString& line, long long lineNum=-1);

private:
    ParseOptions opt_;
    MachineState state_;
    QVector<Move> moves_;

    struct Words { QHash<QChar,double> num; QList<int> gcodes; };



    // —— 工具函数们 ——
    static QString stripComments(const QString& s) {
        // 去掉 ';' 后注释 & 括号注释
        QString out; out.reserve(s.size());
        bool inPar = false;
        for (QChar c : s) {
            if (!inPar && c == ';') break;
            if (c == '(') { inPar = true; continue; }
            if (c == ')') { inPar = false; continue; }
            if (!inPar) out.append(c);
        }
        return out;
    }

    static Words tokenize(const QString& line)
    {
        Words w;

        // 1) 先去掉注释（; 与 ()），避免把注释里的字母数字当成词
        const auto stripComments = [](const QString& s)->QString {
            QString out; out.reserve(s.size());
            bool inPar = false;
            for (QChar ch : s) {
                if (!inPar && ch == ';') break;
                if (ch == '(') { inPar = true; continue; }
                if (ch == ')') { inPar = false; continue; }
                if (!inPar) out.append(ch);
            }
            return out;
        };

        QString s = stripComments(line);

        // 2) 逐个“字母+数值”匹配（允许可选空白、正负号、小数、科学计数）
        //    例：X107.52  Y-258.08  Z17.34  A-80.26  B-5.51  G01  G17.1
        static const QRegularExpression re(
            R"(([A-Za-z])\s*([+\-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+\-]?\d+)?))",
            QRegularExpression::UseUnicodePropertiesOption);

        auto it = re.globalMatch(s);
        while (it.hasNext()) {
            auto m = it.next();
            const QChar c = m.captured(1).toUpper().at(0);
            const QString num = m.captured(2);

            bool ok = false;
            double v = num.toDouble(&ok);
            if (!ok) continue;

            if (c == 'G') {
                // 支持 G01、G1、G17.1 等：保留原排序，取“整数 G”时四舍五入或向下取整
                // 你原来的 Words 里 gcodes 是 int 列表，这里取最近整数：
                int gi = qRound(v);               // 或者：int gi = int(std::floor(v + 1e-9));
                w.gcodes.push_back(gi);
            } else {
                // 普通数值词：X/Y/Z/A/B/C/I/J/K/R/F/S/T 等
                w.num[c] = v; // 多次出现同字母，保留“最后一个”即可（常见做法）
            }
        }

        return w;
    }


    static int lastMotion(const QList<int>& gs) {
        int m = -1;
        for (int g : gs) if (g>=0 && g<=3) m = g;
        return m;
    }

    static double toMM(double v, Unit u) {
        return (u == Unit::MM) ? v : v * 25.4; // inch→mm
    }

    // 在当前平面把 IJK 载入（其它分量置 0）
    static QVector3D projectIJKonPlane(const QVector3D& ijk, Plane p) {
        switch (p) {
        case Plane::XY: return QVector3D(ijk.x(), ijk.y(), 0);
        case Plane::ZX: return QVector3D(ijk.x(), 0, ijk.z());
        case Plane::YZ: return QVector3D(0, ijk.y(), ijk.z());
        }
        return ijk;
    }

    static QVector3D liftCenterTo3D(const QVector3D& c2, const QVector3D& start, Plane p) {
        switch (p) {
        case Plane::XY: return QVector3D(c2.x(), c2.y(), start.z());
        case Plane::ZX: return QVector3D(c2.x(), start.y(), c2.z());
        case Plane::YZ: return QVector3D(start.x(), c2.y(), c2.z());
        }
        return c2;
    }

    static double radiusInPlane(const QVector3D& c, const QVector3D& s, Plane p) {
        switch (p) {
        case Plane::XY: return std::hypot(s.x()-c.x(), s.y()-c.y());
        case Plane::ZX: return std::hypot(s.x()-c.x(), s.z()-c.z());
        case Plane::YZ: return std::hypot(s.y()-c.y(), s.z()-c.z());
        }
        return 0;
    }

    // 通过 R 求圆心（两解：短弧/长弧）
    static QVector3D solveCenterByR(const Move& mv, bool longArc) {
        // 把起终点投影到平面，求中点与法线方向
        auto proj2 = [&](const QVector3D& p)->QVector2D{
            switch (mv.plane) {
            case Plane::XY: return {float(p.x()), float(p.y())};
            case Plane::ZX: return {float(p.x()), float(p.z())};
            case Plane::YZ: return {float(p.y()), float(p.z())};
            } return {0,0};
        };
        QVector2D s = proj2(mv.start), e = proj2(mv.end);
        QVector2D d = e - s;
        double L = std::hypot(d.x(), d.y());
        double R = std::fabs(mv.radius);
        if (L < 1e-9 || R < L/2.0) {
            // 不合法：退化为线
            return (mv.start + mv.end) * 0.5;
        }
        // 距离中点到圆心的偏移
        double h = std::sqrt(R*R - (L*L)/4.0);
        QVector2D m = (s + e) * 0.5f;
        QVector2D n = QVector2D(-d.y(), d.x()).normalized(); // 垂直
        // 选择朝向：短弧与长弧二选一；同时要与 CW/CCW 一致
        QVector2D c2 = m + (longArc ? -1.0 : 1.0) * float(h) * n;

        // 升维到 3D（在平面外坐标用起点分量）
        switch (mv.plane) {
        case Plane::XY: return QVector3D(c2.x(), c2.y(), mv.start.z());
        case Plane::ZX: return QVector3D(c2.x(), mv.start.y(), c2.y());
        case Plane::YZ: return QVector3D(mv.start.x(), c2.x(), c2.y());
        }
        return mv.start;
    }

    struct ArcInfo {
        QVector3D center, u, v;
        double startAngle, endAngle, R;
    };
    static ArcInfo arcPlanarInfo(const Move& mv) {
        // 在平面内建立正交基 {u,v}，计算起终点极角
        auto to2 = [&](const QVector3D& p)->QVector2D{
            switch (mv.plane) {
            case Plane::XY: return {float(p.x()), float(p.y())};
            case Plane::ZX: return {float(p.x()), float(p.z())};
            case Plane::YZ: return {float(p.y()), float(p.z())};
            } return {0,0};
        };
        QVector2D cs = to2(mv.start) - to2(mv.center);
        QVector2D ce = to2(mv.end)   - to2(mv.center);
        double Rs = std::hypot(cs.x(), cs.y());
        double Re = std::hypot(ce.x(), ce.y());
        double R = (Rs + Re) * 0.5;
        double a0 = std::atan2(cs.y(), cs.x());
        double a1 = std::atan2(ce.y(), ce.x());

        // 3D 中的局部轴（仅用于在 3D 里重建点）
        QVector3D u,v;
        switch (mv.plane) {
        case Plane::XY: u = {1,0,0}; v = {0,1,0}; break;
        case Plane::ZX: u = {1,0,0}; v = {0,0,1}; break;
        case Plane::YZ: u = {0,1,0}; v = {0,0,1}; break;
        }
        return { mv.center, u, v, a0, a1, R };
    }

    static double normalizeAngleSigned(double a) {
        // 归一到 (-2π, 2π)
        const double two = 2.0 * M_PI;
        while (a >  M_PI) a -= two;
        while (a < -M_PI) a += two;
        return a;
    }
    static double signedAngle(double a0, double a1, bool ccw) {
        double d = a1 - a0;
        // CCW 取正向，CW 取负向的最短路
        if (ccw) {
            while (d <= 0) d += 2*M_PI;
            while (d >  2*M_PI) d -= 2*M_PI;
        } else {
            while (d >= 0) d -= 2*M_PI;
            while (d < -2*M_PI) d += 2*M_PI;
        }
        return d;
    }
    static QVector3D pointOnCircle(const QVector3D& c,
                                   const QVector3D& u,
                                   const QVector3D& v,
                                   double R, double ang) {
        return c + R*std::cos(ang)*u + R*std::sin(ang)*v;
    }
};

} // namespace NC
