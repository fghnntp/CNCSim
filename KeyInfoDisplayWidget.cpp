#include "KeyInfoDisplayWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFontDatabase>
#include <QGroupBox>

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

    // 运行信息
    QGroupBox *runGroup = new QGroupBox("运行状态");
    QGridLayout *runLayout = new QGridLayout();

    auto addRunItem = [&](InfoType type, const QString &name, const QString &unit = "") {
        infoItems[type].title = new QLabel(name + ":");
        infoItems[type].value = new QLabel("--");
        infoItems[type].unit = unit;

        QHBoxLayout *itemLayout = new QHBoxLayout();
        itemLayout->addWidget(infoItems[type].title);
        itemLayout->addWidget(infoItems[type].value);
        itemLayout->addStretch();

        int row = type - TOOL_NUM;
        runLayout->addLayout(itemLayout, row, 0);
    };

    addRunItem(TOOL_NUM, "当前刀具", "");
    addRunItem(TOOL_LENGTH, "刀具长度补偿", "mm");
    addRunItem(CYCLE_TIME, "循环时间", "");

    runGroup->setLayout(runLayout);
    mainLayout->addWidget(runGroup);

    // 报警信息组
    QGroupBox *alarmGroup = new QGroupBox("报警信息");
    QVBoxLayout *alarmLayout = new QVBoxLayout();

    infoItems[ALARM_MSG].title = nullptr;
    infoItems[ALARM_MSG].value = new QLabel("无报警");
    infoItems[ALARM_MSG].value->setWordWrap(true);
    infoItems[ALARM_MSG].unit = "";

    alarmLayout->addWidget(infoItems[ALARM_MSG].value);
    alarmGroup->setLayout(alarmLayout);
    mainLayout->addWidget(alarmGroup);

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
    infoItems[ALARM_MSG].value->setStyleSheet("color: red;");

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
    for (int mode = S_0; mode < TOOL_NUM; mode++) {
        QString value;
        value = QString::number(active_settings[mode-S_0], 'f', 6);
        infoItems[mode].value->setText(value);
    }
}

void KeyInfoDisplayWidget::updateToolInfo(int toolNum, double toolLength) {
    infoItems[TOOL_NUM].value->setText(QString::number(toolNum));
    infoItems[TOOL_LENGTH].value->setText(QString::number(toolLength, 'f', 3));
}

void KeyInfoDisplayWidget::updateCycleTime(const QString &time) {
    infoItems[CYCLE_TIME].value->setText(time);
}

void KeyInfoDisplayWidget::updateAlarm(const QString &alarmMsg) {
    infoItems[ALARM_MSG].value->setText(alarmMsg.isEmpty() ? "无报警" : alarmMsg);
    infoItems[ALARM_MSG].value->setStyleSheet(alarmMsg.isEmpty() ? "" : "color: red; font-weight: bold;");
}

void KeyInfoDisplayWidget::resetAll() {
    QVector<double> zeros(6, 0.0);
    updatePosition(zeros);
    updateToolInfo(0, 0.0);
    updateCycleTime("00:00:00");
    updateAlarm("");
}
