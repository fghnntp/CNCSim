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
        posLayout->addLayout(itemLayout, row / 3, row % 3);
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

    // 运行信息组
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

        int row = type - FEED_RATE;
        runLayout->addLayout(itemLayout, row, 0);
    };

    addRunItem(FEED_RATE, "进给速度", "mm/min");
    addRunItem(SPINDLE_SPEED, "主轴转速", "rpm");
    addRunItem(TOOL_NUM, "当前刀具", "");
    addRunItem(TOOL_LENGTH, "刀具长度补偿", "mm");
    addRunItem(CYCLE_TIME, "循环时间", "");
    addRunItem(GCODE_MODE, "G代码模式", "");

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

void KeyInfoDisplayWidget::updateFeedRate(double rate) {
    infoItems[FEED_RATE].value->setText(QString::number(rate, 'f', 1));
}

void KeyInfoDisplayWidget::updateSpindleSpeed(double speed) {
    infoItems[SPINDLE_SPEED].value->setText(QString::number(speed, 'f', 0));
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

void KeyInfoDisplayWidget::updateGCodeMode(const QString &mode) {
    infoItems[GCODE_MODE].value->setText(mode);
}

void KeyInfoDisplayWidget::resetAll() {
    QVector<double> zeros(6, 0.0);
    updatePosition(zeros);
    updateFeedRate(0.0);
    updateSpindleSpeed(0.0);
    updateToolInfo(0, 0.0);
    updateCycleTime("00:00:00");
    updateAlarm("");
    updateGCodeMode("--");
}
