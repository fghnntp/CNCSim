#include "KeyInfoDisplayWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFontDatabase>


KeyInfoDisplayWidget::KeyInfoDisplayWidget(QWidget *parent)
    : QWidget(parent) {
    setupUI();
    setupStyle();

    // 初始化定时更新
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, [this]() {
        // 可以在这里添加周期性更新的逻辑
    });
    updateTimer->start(100); // 100ms 更新一次
}

void KeyInfoDisplayWidget::setupUI() {
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    // 初始化所有信息项
    infoItems.resize(INFO_COUNT);

    // 位置信息组
    QGroupBox *posGroup = new QGroupBox("当前位置 (mm)");
    QGridLayout *posLayout = new QGridLayout();

    auto addPosItem = [&](InfoType type, const QString &name) {
        infoItems[type].title = new QLabel(name + ":");
        infoItems[type].value = new QLabel("0.000000");//nm resolution
        infoItems[type].unit = "mm";

        QHBoxLayout *itemLayout = new QHBoxLayout();
        itemLayout->addWidget(infoItems[type].title);
        itemLayout->addWidget(infoItems[type].value);
        itemLayout->addStretch();

        int row = type - POS_X;
        posLayout->addLayout(itemLayout, row / 6, row % 6);
    };

    addPosItem(POS_X, "X轴");
    addPosItem(POS_Y, "Y轴");
    addPosItem(POS_Z, "Z轴");
    addPosItem(POS_A, "A轴");
    addPosItem(POS_B, "B轴");
    addPosItem(POS_C, "C轴");
    addPosItem(POS_U, "U轴");
    addPosItem(POS_V, "V轴");
    addPosItem(POS_W, "W轴");


    posGroup->setLayout(posLayout);
    mainLayout->addWidget(posGroup);
    infoGroups.append(posGroup);

    //解析器G组
    QGroupBox *interGroupG = new QGroupBox("G组");
    QGridLayout *interLayoutG = new QGridLayout();

    auto addInterItemG = [&](InfoType type, const QString &name, const QString &unit = "") {
        infoItems[type].title = new QLabel(name + ":");
        infoItems[type].value = new QLabel("--");
        infoItems[type].unit = unit;

        QHBoxLayout *itemLayout = new QHBoxLayout();
        itemLayout->addWidget(infoItems[type].title);
        itemLayout->addWidget(infoItems[type].value);
        itemLayout->addStretch();

        int row = type - G_0;
        interLayoutG->addLayout(itemLayout, row / 6,  row % 6);
    };

    addInterItemG(G_0, "G0");
    addInterItemG(G_1, "G1");
    addInterItemG(G_2, "G2");
    addInterItemG(G_3, "G3");
    addInterItemG(G_4, "G4");
    addInterItemG(G_5, "G5");
    addInterItemG(G_6, "G6");
    addInterItemG(G_7, "G7");
    addInterItemG(G_8, "G8");
    addInterItemG(G_9, "G9");
    addInterItemG(G_10, "G10");
    addInterItemG(G_11, "G11");
    addInterItemG(G_12, "G12");
    addInterItemG(G_13, "G13");
    addInterItemG(G_14, "G14");
    addInterItemG(G_15, "G15");
    addInterItemG(G_16, "G16");

    interGroupG->setLayout(interLayoutG);
    mainLayout->addWidget(interGroupG);
    infoGroups.append(interGroupG);

    //解析器M组
    QGroupBox *interGroupM = new QGroupBox("M组");
    QGridLayout *interLayoutM = new QGridLayout();

    auto addInterItemM = [&](InfoType type, const QString &name, const QString &unit = "") {
        infoItems[type].title = new QLabel(name + ":");
        infoItems[type].value = new QLabel("--");
        infoItems[type].unit = unit;

        QHBoxLayout *itemLayout = new QHBoxLayout();
        itemLayout->addWidget(infoItems[type].title);
        itemLayout->addWidget(infoItems[type].value);
        itemLayout->addStretch();

        int row = type - M_0;
        interLayoutM->addLayout(itemLayout, row / 6,  row % 6);
    };

    addInterItemM(M_0, "M0");
    addInterItemM(M_1, "M1");
    addInterItemM(M_2, "M2");
    addInterItemM(M_3, "M3");
    addInterItemM(M_4, "M4");
    addInterItemM(M_5, "M5");
    addInterItemM(M_6, "M6");
    addInterItemM(M_7, "M7");
    addInterItemM(M_8, "M8");
    addInterItemM(M_9, "M9");

    interGroupM->setLayout(interLayoutM);
    mainLayout->addWidget(interGroupM);
    infoGroups.append(interGroupM);

    //解析器S组
    QGroupBox *interGroupS = new QGroupBox("S组");
    QGridLayout *interLayoutS = new QGridLayout();

    auto addInterItemS = [&](InfoType type, const QString &name, const QString &unit = "") {
        infoItems[type].title = new QLabel(name + ":");
        infoItems[type].value = new QLabel("--");
        infoItems[type].unit = unit;

        QHBoxLayout *itemLayout = new QHBoxLayout();
        itemLayout->addWidget(infoItems[type].title);
        itemLayout->addWidget(infoItems[type].value);
        itemLayout->addStretch();

        int row = type - S_0;
        interLayoutS->addLayout(itemLayout, row / 6,  row % 6);
    };

    addInterItemS(S_0, "N");
    addInterItemS(S_1, "F");
    addInterItemS(S_2, "S");
    addInterItemS(S_3, "E");
    addInterItemS(S_4, "CE");

    interGroupS->setLayout(interLayoutS);
    mainLayout->addWidget(interGroupS);
    infoGroups.append(interGroupS);

    //Mot信息组
    QGroupBox *motInfoGroup = new QGroupBox("Mot信息");
    QGridLayout *motInfoLayout = new QGridLayout();

    auto addMotInfoItem = [&](InfoType type, const QString &name, const QString &unit = "") {
        infoItems[type].title = new QLabel(name + ":");
        infoItems[type].value = new QLabel("--");
        infoItems[type].unit = unit;

        QHBoxLayout *itemLayout = new QHBoxLayout();
        itemLayout->addWidget(infoItems[type].title);
        itemLayout->addWidget(infoItems[type].value);
        itemLayout->addStretch();

        int row = type - MOT_F_S;
        motInfoLayout->addLayout(itemLayout, row / 6,  row % 6);
    };

    addMotInfoItem(MOT_F_S, "RS/FS");
    addMotInfoItem(MOT_CMD_FB_OK, "CMD/FB");
    addMotInfoItem(MOT_SF_LIMIT, "SL");
    addMotInfoItem(MOT_DTG, "DTG");
    addMotInfoItem(MOT_VEL, "VEL");
    addMotInfoItem(MOT_JOGGING, "JOG");
    addMotInfoItem(MOT_STATE,"S");
    addMotInfoItem(MOT_FLAG,"B");

    motInfoGroup->setLayout(motInfoLayout);
    mainLayout->addWidget(motInfoGroup);
    infoGroups.append(motInfoGroup);

    //Mot模态
    QGroupBox *motModeGroup = new QGroupBox("Mot模态");
    QGridLayout *motModeLayout = new QGridLayout();

    auto addMotModeItem = [&](InfoType type, const QString &name, const QString &unit = "") {
        infoItems[type].title = new QLabel(name + ":");
        infoItems[type].value = new QLabel("--");
        infoItems[type].unit = unit;

        QHBoxLayout *itemLayout = new QHBoxLayout();
        itemLayout->addWidget(infoItems[type].title);
        itemLayout->addWidget(infoItems[type].value);
        itemLayout->addStretch();

        int row = type - MOT_TAG;
        motModeLayout->addLayout(itemLayout, row / 6,  row % 6);
    };
    addMotModeItem(MOT_TAG, "Tag");

    motModeGroup->setLayout(motModeLayout);
    mainLayout->addWidget(motModeGroup);
    infoGroups.append(motModeGroup);

    mainLayout->addStretch();
}

void KeyInfoDisplayWidget::setupStyle() {
    // 设置字体
    QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    fixedFont.setPointSize(9);

    for (const InfoItem &item : infoItems) {
        if (item.value) {
            item.value->setFont(fixedFont);
            item.value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        }
        if (item.title) {
            item.title->setStyleSheet("font-weight: bold;");
        }
    }

    // 报警信息特殊样式
//    infoItems[ALARM_MSG].value->setStyleSheet("color: red;");

    // 整体样式
    setStyleSheet(R"(
        QGroupBox {
            border: 1px solid #ccc;
            border-radius: 3px;
            margin-top: 10px;
            padding-top: 15px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 3px;
        }
    )");
}

// ===== 更新函数 =====

void KeyInfoDisplayWidget::updatePosition(const QVector<double> &pos) {
    if (pos.size() >= 1) infoItems[POS_X].value->setText(QString::number(pos[0], 'f', 6));
    if (pos.size() >= 2) infoItems[POS_Y].value->setText(QString::number(pos[1], 'f', 6));
    if (pos.size() >= 3) infoItems[POS_Z].value->setText(QString::number(pos[2], 'f', 6));
    if (pos.size() >= 4) infoItems[POS_A].value->setText(QString::number(pos[3], 'f', 6));
    if (pos.size() >= 5) infoItems[POS_B].value->setText(QString::number(pos[4], 'f', 6));
    if (pos.size() >= 6) infoItems[POS_C].value->setText(QString::number(pos[5], 'f', 6));
    if (pos.size() >= 7) infoItems[POS_C].value->setText(QString::number(pos[6], 'f', 6));
    if (pos.size() >= 8) infoItems[POS_C].value->setText(QString::number(pos[7], 'f', 6));
    if (pos.size() >= 9) infoItems[POS_C].value->setText(QString::number(pos[8], 'f', 6));
}

void KeyInfoDisplayWidget::updateGModes(int active_gcodes[])
{
    for (int mode = G_0; mode < M_0; mode++) {
        QString value = "G";
        value += QString::number(active_gcodes[mode-G_0] / 10);
        if (mode != G_1 && value == "G0") {
            infoItems[mode].value->setText("--");
            continue;
        }
        int sub = active_gcodes[mode-G_0] % 10;
        value += ((sub <= 0) ? ("") : ("." + QString::number(sub)));

        infoItems[mode].value->setText(value);
    }
}

void KeyInfoDisplayWidget::updateMModes(int active_mcodes[])
{
    for (int mode = M_0; mode < S_0; mode++) {
        QString value = "M";
        value += QString::number(active_mcodes[mode-M_0]);
        if (active_mcodes[mode-M_0] < 0) {
            infoItems[mode].value->setText("--");
            continue;
        }
        infoItems[mode].value->setText(value);
    }
}

void KeyInfoDisplayWidget::updateSModes(double active_settings[])
{
    for (int mode = S_0; mode < MOT_F_S; mode++) {
        QString value;
        value = QString::number(active_settings[mode-S_0], 'f', 6);
        infoItems[mode].value->setText(value);
    }
}

void KeyInfoDisplayWidget::updateScale(double rapid_scale, double feed_scale)
{
    QString value;
    value += QString::number(static_cast<int>(rapid_scale * 100));
    value += "%/";
    value += QString::number(static_cast<int>(feed_scale * 100));
    value += "%";
    infoItems[MOT_F_S].value->setText(value);
}

void KeyInfoDisplayWidget::updateCmdFb(int cmd, int fb)
{
    QString value;
    value += QString::number(cmd) + "/" + QString::number(fb);
    infoItems[MOT_CMD_FB_OK].value->setText(value);
}

void KeyInfoDisplayWidget::updateSoftLimit(int limit)
{
    QString value;
    value += QString::number(limit);
    infoItems[MOT_SF_LIMIT].value->setText(value);
}

void KeyInfoDisplayWidget::updateDtg(double dtg)
{
    QString value;
    value += QString::number(dtg, 'f', 2);
    infoItems[MOT_DTG].value->setText(value);
}

void KeyInfoDisplayWidget::updateRuninfo(double cur_vel, double req_vel)
{
    infoItems[MOT_VEL].value->setText(
                QString::number(cur_vel, 'f', 2) + "/" +
                QString::number(req_vel, 'f', 2));
}

void KeyInfoDisplayWidget::updateJogging(int jogging)
{
    infoItems[MOT_JOGGING].value->setText(
                QString::number(jogging));
}

void KeyInfoDisplayWidget::updateState(int state)
{
    QString value;

    if (state == 0) {
        value = "DISABLE";
    }
    else if (state == 1) {
        value = "FREE";
    }
    else if (state == 2) {
        value = "TELEOP";
    }
    else if (state == 3) {
        value = "COOR";
    }
    else {
        value = "WRONG";
    }
    infoItems[MOT_STATE].value->setText(value);
}


void KeyInfoDisplayWidget::updateMotionFlag(int flag)
{
    /* bit masks */
    #define EMCMOT_MOTION_ENABLE_BIT      0x0001
    #define EMCMOT_MOTION_INPOS_BIT       0x0002
    #define EMCMOT_MOTION_COORD_BIT       0x0004
    #define EMCMOT_MOTION_ERROR_BIT       0x0008
    #define EMCMOT_MOTION_TELEOP_BIT      0x0010

    QString value;
    if (flag & EMCMOT_MOTION_ENABLE_BIT) {
        value += "ena ";
    }
    else {
        value += "dis ";
    }

    if (flag & EMCMOT_MOTION_INPOS_BIT) {
        value += "inpos ";
    }
    else {
        value += "noInpos ";
    }

    if (flag & EMCMOT_MOTION_COORD_BIT) {
        value += "coord ";
    }
    else {
        value += "noCoord ";
    }

    if (flag & EMCMOT_MOTION_ERROR_BIT) {
        value += "error ";
    }
    else {
        value += "noErr ";
    }

    if (flag & EMCMOT_MOTION_TELEOP_BIT) {
        value += "teleop ";
    }
    else {
        value += "noTeleop ";
    }

    infoItems[MOT_FLAG].value->setText(value);
}

std::string getModesStr(struct state_tag_t &tag)
{
    std::stringstream ss;
    if (tag.packed_flags & GM_FLAG_UNITS) {
        ss << "GM_FLAG_UNITS\n";
    }
    if (tag.packed_flags & GM_FLAG_DISTANCE_MODE) {
        ss << "GM_FLAG_DISTANCE_MODE\n";
    }
    if (tag.packed_flags & GM_FLAG_TOOL_OFFSETS_ON) {
        ss << "GM_FLAG_TOOL_OFFSETS_ON\n";
    }
    if (tag.packed_flags & GM_FLAG_RETRACT_OLDZ) {
        ss << "GM_FLAG_RETRACT_OLDZ\n";
    }
    if (tag.packed_flags & GM_FLAG_BLEND) {
        ss << "GM_FLAG_BLEND\n";
    }
    if (tag.packed_flags & GM_FLAG_EXACT_STOP) {
        ss << "GM_FLAG_EXACT_STOP\n";
    }
    if (tag.packed_flags & GM_FLAG_FEED_INVERSE_TIME) {
        ss << "GM_FLAG_FEED_INVERSE_TIME\n";
    }
    if (tag.packed_flags & GM_FLAG_FEED_UPM) {
        ss << "GM_FLAG_FEED_UPM\n";
    }
    if (tag.packed_flags & GM_FLAG_CSS_MODE) {
        ss << "GM_FLAG_CSS_MODE\n";
    }
    if (tag.packed_flags & GM_FLAG_IJK_ABS) {
        ss << "GM_FLAG_IJK_ABS\n";
    }
    if (tag.packed_flags & GM_FLAG_DIAMETER_MODE) {
        ss << "GM_FLAG_DIAMETER_MODE\n";
    }
    if (tag.packed_flags & GM_FLAG_G92_IS_APPLIED) {
        ss << "GM_FLAG_G92_IS_APPLIED\n";
    }
    if (tag.packed_flags & GM_FLAG_SPINDLE_ON) {
        ss << "GM_FLAG_SPINDLE_ON\n";
    }
    if (tag.packed_flags & GM_FLAG_SPINDLE_CW) {
        ss << "GM_FLAG_SPINDLE_CW\n";
    }
    if (tag.packed_flags & GM_FLAG_MIST) {
        ss << "GM_FLAG_MIST\n";
    }
    if (tag.packed_flags & GM_FLAG_FLOOD) {
        ss << "GM_FLAG_FLOOD\n";
    }
    if (tag.packed_flags & GM_FLAG_FEED_OVERRIDE) {
        ss << "GM_FLAG_FEED_OVERRIDE\n";
    }
    if (tag.packed_flags & GM_FLAG_SPEED_OVERRIDE) {
        ss << "GM_FLAG_SPEED_OVERRIDE\n";
    }
    if (tag.packed_flags & GM_FLAG_ADAPTIVE_FEED) {
        ss << "GM_FLAG_ADAPTIVE_FEED\n";
    }
    if (tag.packed_flags & GM_FLAG_FEED_HOLD) {
        ss << "GM_FLAG_FEED_HOLD\n";
    }
    if (tag.packed_flags & GM_FLAG_RESTORABLE) {
        ss << "GM_FLAG_RESTORABLE\n";
    }
    if (tag.packed_flags & GM_FLAG_IN_REMAP) {
        ss << "GM_FLAG_IN_REMAP\n";
    }
    if (tag.packed_flags & GM_FLAG_IN_SUB) {
        ss << "GM_FLAG_IN_SUB\n";
    }
    if (tag.packed_flags & GM_FLAG_EXTERNAL_FILE) {
        ss << "GM_FLAG_EXTERNAL_FILE\n";
    }
    return ss.str();
}

#include <iomanip>
std::string getModesDetialStr(struct state_tag_t &tag)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(6);

    ss << "LINE_NUMBER: " << tag.fields[GM_FIELD_LINE_NUMBER] << "\n" <<
          "G_MODE_0: "  << tag.fields[GM_FIELD_G_MODE_0] << "\n" <<
          "CUTTER_COMP: " << tag.fields[GM_FIELD_CUTTER_COMP] << "\n" <<
          "MOTION_MODE: " << tag.fields[GM_FIELD_MOTION_MODE] << "\n" <<
          "PLANE: " << tag.fields[GM_FIELD_PLANE] << "\n" <<
          "M_MODES_4: " << tag.fields[GM_FIELD_M_MODES_4] << "\n" <<
          "ORIGIN: " << tag.fields[GM_FIELD_ORIGIN] << "\n" <<
          "TOOLCHANGE: " << tag.fields[GM_FIELD_TOOLCHANGE] << "\n" <<
          "FLOAT_LINE_NUMBER: " << tag.fields_float[GM_FIELD_FLOAT_LINE_NUMBER] << "\n" <<
          "FLOAT_FEED: " << tag.fields_float[GM_FIELD_FLOAT_FEED] << "\n" <<
          "FLOAT_SPEED: " << tag.fields_float[GM_FIELD_FLOAT_SPEED] << "\n" <<
          "FLOAT_PATH_TOLERANCE: " << tag.fields_float[GM_FIELD_FLOAT_PATH_TOLERANCE] << "\n" <<
          "FLOAT_NAIVE_CAM_TOLERANCE: " << tag.fields_float[GM_FIELD_FLOAT_NAIVE_CAM_TOLERANCE];

    return ss.str();
}

void KeyInfoDisplayWidget::updateTag(state_tag_t &tag)
{
    QString value;
    QString filename = tag.filename;
    if (!filename.isEmpty())
        value += filename + "\n";

    value += QString::fromStdString(getModesStr(tag));
    value += QString::fromStdString(getModesDetialStr(tag));
    infoItems[MOT_TAG].value->setText(value);
}

int KeyInfoDisplayWidget::showHideInfo(InfoGroup index, bool show)
{
    if (index < 0 || index > kInfoGroupNum)
        return 1;

    if (show)
        infoGroups[index]->show();
    else
        infoGroups[index]->hide();

    return 0;
}

void KeyInfoDisplayWidget::resetAll() {
    QVector<double> zeros(6, 0.0);
    updatePosition(zeros);
}
