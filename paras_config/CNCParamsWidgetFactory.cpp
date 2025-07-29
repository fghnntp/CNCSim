#include "CNCParamsWidgetFactory.h"
#include "TrajParamsWidget.h"
#include "KinematicParamsWidget.h"
//#include "JointParamsWidget.h"
#include "AxisParamsWidget.h"
//#include "SpindleParamsWidget.h"

CNCParamsWidgetFactory::CNCParamsWidgetFactory(const QVariantMap &globalConfig)
    : config(globalConfig) {}

BaseParamsWidget* CNCParamsWidgetFactory::createTrajWidget(QWidget *parent) {
    TrajParamsWidget *widget = new TrajParamsWidget(parent);
    widget->loadConfig(getTrajConfig());
    return widget;
}

BaseParamsWidget* CNCParamsWidgetFactory::createKinematicWidget(const QString &name, QWidget *parent) {
    KinematicParamsWidget *widget = new KinematicParamsWidget(name, parent);
    widget->loadConfig(getKinematicConfig(name));
    return widget;
}

// ... 其他create方法类似实现 ...

QVariantMap CNCParamsWidgetFactory::getTrajConfig() const {
    return config.value("traj").toMap();
}

QVariantMap CNCParamsWidgetFactory::getKinematicConfig(const QString &name) const {
    const QVariantList kinematics = config.value("kinematics").toList();
    for (const QVariant &item : kinematics) {
        QVariantMap kin = item.toMap();
        if (kin.value("name").toString() == name) {
            return kin;
        }
    }
    return QVariantMap();
}

// ... 其他getConfig方法类似实现 ...

void CNCParamsWidgetFactory::updateConfig(const QVariantMap &newConfig) {
    config = newConfig;
}
