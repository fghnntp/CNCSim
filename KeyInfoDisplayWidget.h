#ifndef _KEYINFO_DISPLAY_WIDGET_H_
#define _KEYINFO_DISPLAY_WIDGET_H_

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QTimer>
#include <QVector>
#include "state_tag.h"

class KeyInfoDisplayWidget : public QWidget {
    Q_OBJECT

public:
    explicit KeyInfoDisplayWidget(QWidget *parent = nullptr);

    // 更新关键信息
    void updatePosition(const QVector<double> &pos);
    void updateGModes(int active_gcodes[]);
    void updateMModes(int active_mcodes[]);
    void updateSModes(double active_settings[]);
    void updateScale(double rapid_scale, double feed_scale);
    void updateCmdFb(int cmd, int fb);
    void updateSoftLimit(int limit);
    void updateDtg(double dtg);
    void updateRuninfo(double cur_vel, double req_vel);
    void updateJogging(int jogging);
    void updateState(int state);
    void updateMotionFlag(int flag);
    void updateTag(state_tag_t &tag);

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
        MOT_F_S, MOT_CMD_FB_OK, MOT_SF_LIMIT, MOT_DTG, MOT_VEL, MOT_JOGGING, MOT_STATE, MOT_FLAG,
        MOT_TAG,
        INFO_COUNT
    };

    QVector<InfoItem> infoItems;
    QTimer *updateTimer;
};

#endif
