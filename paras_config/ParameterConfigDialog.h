#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QPushButton>
#include <QStackedWidget>
#include "ParamsWidgetFactory.h"

class ParameterConfigDialog : public QDialog {
    Q_OBJECT
public:
    explicit ParameterConfigDialog(ParamsWidgetFactory *factory, QWidget *parent = nullptr);

    void loadCurrentConfig();
    void saveToSystem();
    void saveToFile(const QString &path);
    void loadFromFile(const QString &path);

private slots:
    void onApply();
    void onSave();
    void onLoad();
    void onKinematicChanged(int index);
    void onJointChanged(int index);
    void onAxisChanged(int index);
    void onSpindleChanged(int index);
    void onConfigModified();

private:
    void setupUI();
    void updateModifiedStatus();

    ParamsWidgetFactory *factory;

    // 主控件
    QTabWidget *tabWidget;
    QStackedWidget *kinematicStack;
    QStackedWidget *jointStack;
    QStackedWidget *axisStack;
    QStackedWidget *spindleStack;

    // 子选择器
    QComboBox *cmbKinematics;
    QComboBox *cmbJoints;
    QComboBox *cmbAxes;
    QComboBox *cmbSpindles;

    // 按钮
    QPushButton *btnApply;
    QPushButton *btnSave;
    QPushButton *btnLoad;
    QPushButton *btnOk;
    QPushButton *btnCancel;

    // 参数控件
    BaseParamsWidget *trajWidget;
    QList<BaseParamsWidget*> kinematicWidgets;
    QList<BaseParamsWidget*> jointWidgets;
    QList<BaseParamsWidget*> axisWidgets;
    QList<BaseParamsWidget*> spindleWidgets;

    // 状态
    bool isModified = false;
};
