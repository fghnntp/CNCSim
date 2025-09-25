#include "NCPaser.h"

using namespace::NC;

// —— 行解析 ——
void Parser::parseLine(const QString& line, long long lineNum) {
    // 支持一行多个词（G/X/Y/Z/I/J/K/R/F 等），G 可多次改变 modal
    Words w = tokenize(line);

    // 处理 modal：单位/增量/平面
    for (int g : w.gcodes) {
        switch (g) {
        case 20: state_.unit = Unit::INCH; break;
        case 21: state_.unit = Unit::MM;   break;
        case 90: state_.dist = DistanceMode::ABSOLUTE;   break;
        case 91: state_.dist = DistanceMode::INCREMENTAL;break;
        case 17: state_.plane = Plane::XY;  break;
        case 18: state_.plane = Plane::ZX;  break;
        case 19: state_.plane = Plane::YZ;  break;
        default: break;
        }
    }
    if (w.num.contains('F')) state_.feed = toMM(w.num['F'], state_.unit);
    if (w.num.contains('T')) {
        state_.toolNum = int(w.num['T'] + 0.001);
    }


    // 运动：G0/G1/G2/G3（取最后出现的那个作为本行生效）
    int motion = lastMotion(w.gcodes);
    if (motion < 0) {
    }

    // 目标位（按增量/绝对）
    QVector3D target = state_.pos;
    auto setAxis = [&](QChar ax, int idx){
        if (!w.num.contains(ax)) return;
        double v = toMM(w.num[ax], state_.unit);
        if (state_.dist == DistanceMode::ABSOLUTE) target[idx] = v;
        else target[idx] += v;
    };
    setAxis('X', 0); setAxis('Y', 1); setAxis('Z', 2);

    Move mv;
    mv.start = state_.pos;
    mv.end = target;
    mv.feed = state_.feed;
    mv.plane = state_.plane;
    mv.toolNum = state_.toolNum;
    mv.lineNum = lineNum;

    switch (motion) {
    case 0: mv.type = MoveType::Rapid;  moves_.push_back(mv); state_.pos = target; return;
    case 1: mv.type = MoveType::Linear; moves_.push_back(mv); state_.pos = target; return;
    case 2: mv.type = MoveType::ArcCW;  break;
    case 3: mv.type = MoveType::ArcCCW; break;
    }

    // —— 圆弧：IJK 或 R ——
    // 计算圆心（绝对）
    bool hasR = w.num.contains('R');
    if (hasR) {
        mv.usesRadius = true;
        mv.radius = toMM(w.num['R'], state_.unit);
        mv.center = solveCenterByR(mv, opt_.allowHugeRArc);
    } else {
        mv.usesRadius = false;
        QVector3D ijk(0,0,0);
        if (w.num.contains('I')) ijk.setX(toMM(w.num['I'], state_.unit));
        if (w.num.contains('J')) ijk.setY(toMM(w.num['J'], state_.unit));
        if (w.num.contains('K')) ijk.setZ(toMM(w.num['K'], state_.unit));
        QVector3D center = mv.start;
        if (opt_.ijkIncremental) { center += projectIJKonPlane(ijk, state_.plane); }
        else { // 绝对 IJK（少见）
            center = projectIJKonPlane(ijk, state_.plane);
            center = liftCenterTo3D(center, mv.start, state_.plane);
        }
        mv.center = center;
        mv.radius = radiusInPlane(mv.center, mv.start, state_.plane);
    }

    moves_.push_back(mv);
    state_.pos = target;
}
