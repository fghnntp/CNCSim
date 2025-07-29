#pragma once

#include "BaseParamsWidget.h"
#include <QCheckBox>

class AxisParamsWidget : public BaseParamsWidget {
    Q_OBJECT
public:
    explicit AxisParamsWidget(const QString &name, QWidget *parent = nullptr);

    void loadConfig(const QVariantMap &config) override;
    QVariantMap getConfig() const override;

    QString getName() const;

private:
    QString name;
    QDoubleSpinBox *spnStepsPerUnit;
    QDoubleSpinBox *spnMaxTravel;
    QDoubleSpinBox *spnHomeOffset;
    QCheckBox *chkInvertDirection;
};
