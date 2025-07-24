#ifndef _KEYINFO_DISPLAY_WIDGET_H_
#define _KEYINFO_DISPLAY_WIDGET_H_

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QTimer>
#include <QVector>

class KeyInfoDisplayWidget : public QWidget {
    Q_OBJECT

public:
    explicit KeyInfoDisplayWidget(QWidget *parent = nullptr);

    // 更新关键信息
    void updatePosition(const QVector<double> &pos);
    void updateFeedRate(double rate);
    void updateSpindleSpeed(double speed);
    void updateToolInfo(int toolNum, double toolLength);
    void updateCycleTime(const QString &time);
    void updateAlarm(const QString &alarmMsg);
    void updateGCodeMode(const QString &mode);

    // 重置所有信息
    void resetAll();

private:
    void setupUI();
    void setupStyle();

    // 信息项结构
    struct InfoItem {
        QLabel *title;
        QLabel *value;
        QString unit;
    };

    // 信息项枚举
    enum InfoType {
        POS_X, POS_Y, POS_Z, POS_A, POS_B, POS_C, POS_U, POS_V, POS_W,
        FEED_RATE, SPINDLE_SPEED, TOOL_NUM, TOOL_LENGTH,
        CYCLE_TIME, GCODE_MODE, ALARM_MSG,
        INFO_COUNT
    };

    QVector<InfoItem> infoItems;
    QTimer *updateTimer;
};

#endif
