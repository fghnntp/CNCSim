#pragma once

#include <QWidget>
#include <QVariantMap>
#include <QFormLayout>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QSet>

class BaseParamsWidget : public QWidget {
    Q_OBJECT
public:
    explicit BaseParamsWidget(const QString &title, QWidget *parent = nullptr);

    virtual void loadConfig(const QVariantMap &config) = 0;
    virtual QVariantMap getConfig() const = 0;
    virtual bool validate() const;

    void markAsModified(bool modified = true);
    bool isModified() const;

signals:
    void configChanged();

protected:
    void trackChanges(QWidget *widget);
    void setupUI(const QVariantMap &defaultValues);

    QFormLayout *formLayout;
    QVariantMap originalConfig;
    bool modified = false;

private slots:
    void onWidgetModified();

private:
    QSet<QWidget*> trackedWidgets;
};

template<typename T>
class ParamControl {
public:
    static T* create(QWidget *parent, const QString &name, const QVariant &defaultValue) {
        T *widget = new T(parent);
        if constexpr (std::is_same_v<T, QLineEdit>) {
            widget->setText(defaultValue.toString());
        } else if constexpr (std::is_same_v<T, QSpinBox> || std::is_same_v<T, QDoubleSpinBox>) {
            widget->setValue(defaultValue.toDouble());
        } else if constexpr (std::is_same_v<T, QComboBox>) {
            widget->addItems(defaultValue.toStringList());
        }
        return widget;
    }
};
