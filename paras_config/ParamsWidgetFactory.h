#pragma once

#include "BaseParamsWidget.h"
#include <QVariantMap>

class ParamsWidgetFactory {
public:
    virtual BaseParamsWidget* createTrajWidget(QWidget *parent) = 0;
    virtual BaseParamsWidget* createKinematicWidget(const QString &name, QWidget *parent) = 0;
    virtual BaseParamsWidget* createJointWidget(const QString &name, QWidget *parent) = 0;
    virtual BaseParamsWidget* createAxisWidget(const QString &name, QWidget *parent) = 0;
    virtual BaseParamsWidget* createSpindleWidget(const QString &name, QWidget *parent) = 0;
    
    virtual QStringList getKinematicNames() const = 0;
    virtual QStringList getJointNames() const = 0;
    virtual QStringList getAxisNames() const = 0;
    virtual QStringList getSpindleNames() const = 0;
    
    virtual void updateConfig(const QVariantMap &newConfig) = 0;
    virtual QVariantMap getCurrentSystemConfig() const = 0;
    virtual QVariantMap getTrajConfig() const = 0;
    virtual QVariantMap getKinematicConfig(const QString &name) const = 0;
    virtual QVariantMap getJointConfig(const QString &name) const = 0;
    virtual QVariantMap getAxisConfig(const QString &name) const = 0;
    virtual QVariantMap getSpindleConfig(const QString &name) const = 0;
    
    virtual ~ParamsWidgetFactory() = default;
};