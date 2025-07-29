#pragma once

#include "BaseParamsWidget.h"

class TrajParamsWidget : public BaseParamsWidget {
    Q_OBJECT
public:
    explicit TrajParamsWidget(QWidget *parent = nullptr);

    void loadConfig(const QVariantMap &config) override;
    QVariantMap getConfig() const override;

private:
    QDoubleSpinBox *spnAcceleration;
    QDoubleSpinBox *spnJerk;
    QDoubleSpinBox *spnCorneringFactor;
    QDoubleSpinBox *spnMaxVelocity;
};
