#include "BaseParamsWidget.h"
#include <QLabel>

BaseParamsWidget::BaseParamsWidget(const QString &title, QWidget *parent)
    : QWidget(parent), formLayout(new QFormLayout(this)) {
    if (!title.isEmpty()) {
        formLayout->addRow(new QLabel("<b>" + title + "</b>"));
    }
}

void BaseParamsWidget::trackChanges(QWidget *widget) {
    if (auto edit = qobject_cast<QLineEdit*>(widget)) {
        connect(edit, &QLineEdit::textEdited, this, &BaseParamsWidget::onWidgetModified);
    }
    else if (auto spin = qobject_cast<QAbstractSpinBox*>(widget)) {
        connect(spin, QOverload<>::of(&QAbstractSpinBox::editingFinished),
                this, &BaseParamsWidget::onWidgetModified);
    }
    else if (auto combo = qobject_cast<QComboBox*>(widget)) {
        connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &BaseParamsWidget::onWidgetModified);
    }
    trackedWidgets.insert(widget);
}

void BaseParamsWidget::onWidgetModified() {
    markAsModified(true);
    emit configChanged();
}

void BaseParamsWidget::markAsModified(bool modified) {
    this->modified = modified;
    QPalette pal = palette();
    pal.setColor(QPalette::Window, modified ? QColor(255, 255, 200) : Qt::white);
    setAutoFillBackground(true);
    setPalette(pal);
}

bool BaseParamsWidget::isModified() const {
    return modified;
}

bool BaseParamsWidget::validate() const {
    return true;
}
