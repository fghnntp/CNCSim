#include "TrajParamsWidget.h"

TrajParamsWidget::TrajParamsWidget(QWidget *parent)
    : BaseParamsWidget("轨迹参数", parent) {
    spnAcceleration = ParamControl<QDoubleSpinBox>::create(this, "acceleration", 1000.0);
    spnJerk = ParamControl<QDoubleSpinBox>::create(this, "jerk", 500.0);
    spnCorneringFactor = ParamControl<QDoubleSpinBox>::create(this, "cornering", 0.1);
    spnMaxVelocity = ParamControl<QDoubleSpinBox>::create(this, "max_velocity", 3000.0);

    formLayout->addRow("最大加速度 (mm/s²):", spnAcceleration);
    formLayout->addRow("加加速度 (mm/s³):", spnJerk);
    formLayout->addRow("拐角因子:", spnCorneringFactor);
    formLayout->addRow("最大速度 (mm/min):", spnMaxVelocity);

    trackChanges(spnAcceleration);
    trackChanges(spnJerk);
    trackChanges(spnCorneringFactor);
    trackChanges(spnMaxVelocity);
}

void TrajParamsWidget::loadConfig(const QVariantMap &config) {
    spnAcceleration->setValue(config.value("acceleration", 1000.0).toDouble());
    spnJerk->setValue(config.value("jerk", 500.0).toDouble());
    spnCorneringFactor->setValue(config.value("cornering", 0.1).toDouble());
    spnMaxVelocity->setValue(config.value("max_velocity", 3000.0).toDouble());

    originalConfig = config;
    markAsModified(false);
}

QVariantMap TrajParamsWidget::getConfig() const {
    QVariantMap config;
    config["acceleration"] = spnAcceleration->value();
    config["jerk"] = spnJerk->value();
    config["cornering"] = spnCorneringFactor->value();
    config["max_velocity"] = spnMaxVelocity->value();
    return config;
}
