#pragma once

#include "BaseParamsWidget.h"

class KinematicParamsWidget : public BaseParamsWidget {
    Q_OBJECT
public:
    explicit KinematicParamsWidget(const QString &name, QWidget *parent = nullptr);

    void loadConfig(const QVariantMap &config) override;
    QVariantMap getConfig() const override;

    QString getName() const;

private:
    QString name;
    QLineEdit *txtType;
    QDoubleSpinBox *spnMaxVelocity;
    QDoubleSpinBox *spnHomePosition;
    QComboBox *cmbAlgorithm;
};
