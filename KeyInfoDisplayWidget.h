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
    void updateGModes(int active_gcodes[]);
    void updateMModes(int active_mcodes[]);
    void updateSModes(double active_settings[]);

    void updateToolInfo(int toolNum, double toolLength);
    void updateCycleTime(const QString &time);
    void updateAlarm(const QString &alarmMsg);

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
        G_0, G_1, G_2, G_3, G_4, G_5, G_6, G_7, G_8, G_9, G_10, G_11, G_12, G_13, G_14, G_15, G_16,
        M_0, M_1, M_2, M_3, M_4, M_5, M_6, M_7, M_8, M_9,
        S_0, S_1, S_2, S_3, S_4,
        TOOL_NUM, TOOL_LENGTH, CYCLE_TIME,
        ALARM_MSG,
        INFO_COUNT
    };

    QVector<InfoItem> infoItems;
    QTimer *updateTimer;
};

#endif
