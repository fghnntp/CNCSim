#pragma once

#include "ParamsWidgetFactory.h"
#include <QMap>

class CNCParamsWidgetFactory : public ParamsWidgetFactory {
public:
    explicit CNCParamsWidgetFactory(const QVariantMap &globalConfig);
    
    // 实现所有纯虚函数
    BaseParamsWidget* createTrajWidget(QWidget *parent) override;
    BaseParamsWidget* createKinematicWidget(const QString &name, QWidget *parent) override;
    BaseParamsWidget* createJointWidget(const QString &name, QWidget *parent) override;
    BaseParamsWidget* createAxisWidget(const QString &name, QWidget *parent) override;
    BaseParamsWidget* createSpindleWidget(const QString &name, QWidget *parent) override;
    
    QStringList getKinematicNames() const override;
    QStringList getJointNames() const override;
    QStringList getAxisNames() const override;
    QStringList getSpindleNames() const override;
    
    void updateConfig(const QVariantMap &newConfig) override;
    QVariantMap getCurrentSystemConfig() const override;
    
    QVariantMap getTrajConfig() const override;
    QVariantMap getKinematicConfig(const QString &name) const override;
    QVariantMap getJointConfig(const QString &name) const override;
    QVariantMap getAxisConfig(const QString &name) const override;
    QVariantMap getSpindleConfig(const QString &name) const override;
    
private:
    QVariantMap config;
};