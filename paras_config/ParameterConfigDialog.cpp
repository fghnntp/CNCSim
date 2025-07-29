#include "ParameterConfigDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>

ParameterConfigDialog::ParameterConfigDialog(ParamsWidgetFactory *factory, QWidget *parent)
    : QDialog(parent), factory(factory) {
    setupUI();
    loadCurrentConfig();
}

void ParameterConfigDialog::setupUI() {
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 标签页
    tabWidget = new QTabWidget;

    // 1. 轨迹参数标签
    trajWidget = factory->createTrajWidget(this);
    tabWidget->addTab(trajWidget, "轨迹参数");

    // 2. 运动学参数标签
    QWidget *kinematicTab = new QWidget;
    QVBoxLayout *kinematicLayout = new QVBoxLayout(kinematicTab);

    cmbKinematics = new QComboBox;
    cmbKinematics->addItems(factory->getKinematicNames());
    kinematicLayout->addWidget(cmbKinematics);

    kinematicStack = new QStackedWidget;
    kinematicLayout->addWidget(kinematicStack);

    // 初始化所有运动学界面
    for (const QString &name : factory->getKinematicNames()) {
        BaseParamsWidget *widget = factory->createKinematicWidget(name, this);
        kinematicWidgets.append(widget);
        kinematicStack->addWidget(widget);
    }

    tabWidget->addTab(kinematicTab, "运动学参数");

    // ... 其他参数标签类似设置 ...

    // 按钮区
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    btnApply = new QPushButton("应用");
    btnSave = new QPushButton("保存到文件");
    btnLoad = new QPushButton("从文件加载");
    btnOk = new QPushButton("确定");
    btnCancel = new QPushButton("取消");

    buttonLayout->addWidget(btnApply);
    buttonLayout->addWidget(btnSave);
    buttonLayout->addWidget(btnLoad);
    buttonLayout->addStretch();
    buttonLayout->addWidget(btnOk);
    buttonLayout->addWidget(btnCancel);

    mainLayout->addWidget(tabWidget);
    mainLayout->addLayout(buttonLayout);

    // 连接信号
    connect(btnApply, &QPushButton::clicked, this, &ParameterConfigDialog::onApply);
    connect(btnSave, &QPushButton::clicked, this, &ParameterConfigDialog::onSave);
    connect(btnLoad, &QPushButton::clicked, this, &ParameterConfigDialog::onLoad);
    connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    connect(cmbKinematics, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ParameterConfigDialog::onKinematicChanged);

    // 监听所有参数变化
    auto connectConfigChanged = [this](BaseParamsWidget *widget) {
        connect(widget, &BaseParamsWidget::configChanged,
                this, &ParameterConfigDialog::onConfigModified);
    };

    connectConfigChanged(trajWidget);
    for (BaseParamsWidget *widget : kinematicWidgets) {
        connectConfigChanged(widget);
    }
    // ... 连接其他参数控件 ...
}

void ParameterConfigDialog::loadCurrentConfig() {
    // 从工厂加载当前配置
    factory->updateConfig(factory->getCurrentSystemConfig());

    // 重新加载所有界面
    trajWidget->loadConfig(factory->getTrajConfig());

    for (int i = 0; i < kinematicWidgets.size(); ++i) {
        kinematicWidgets[i]->loadConfig(factory->getKinematicConfig(
            factory->getKinematicNames()[i]));
    }
    // ... 加载其他参数 ...

    isModified = false;
    updateModifiedStatus();
}

void ParameterConfigDialog::saveToSystem() {
    QVariantMap newConfig;

    newConfig["traj"] = trajWidget->getConfig();

    QVariantList kinematics;
    for (int i = 0; i < kinematicWidgets.size(); ++i) {
        kinematics.append(kinematicWidgets[i]->getConfig());
    }
    newConfig["kinematics"] = kinematics;

    // ... 收集其他参数 ...

    factory->updateConfig(newConfig);
    isModified = false;
    updateModifiedStatus();
}

void ParameterConfigDialog::saveToFile(const QString &path) {
    QVariantMap config;

    config["traj"] = trajWidget->getConfig();

    QVariantList kinematics;
    for (BaseParamsWidget *widget : kinematicWidgets) {
        kinematics.append(widget->getConfig());
    }
    config["kinematics"] = kinematics;

    // ... 收集其他参数 ...

    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(QJsonObject::fromVariantMap(config));
        file.write(doc.toJson());
        file.close();
    }
}

void ParameterConfigDialog::loadFromFile(const QString &path) {
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QVariantMap fileConfig = doc.object().toVariantMap();

        // 只更新工厂配置，不直接应用到系统
        factory->updateConfig(fileConfig);

        // 重新加载界面
        loadCurrentConfig();
    }
}

void ParameterConfigDialog::onApply() {
    saveToSystem();
}

void ParameterConfigDialog::onSave() {
    QString path = QFileDialog::getSaveFileName(this, "保存配置文件", "", "JSON文件 (*.json)");
    if (!path.isEmpty()) {
        saveToFile(path);
    }
}

void ParameterConfigDialog::onLoad() {
    QString path = QFileDialog::getOpenFileName(this, "加载配置文件", "", "JSON文件 (*.json)");
    if (!path.isEmpty()) {
        loadFromFile(path);
    }
}

void ParameterConfigDialog::onConfigModified() {
    isModified = true;
    updateModifiedStatus();
}

void ParameterConfigDialog::updateModifiedStatus() {
    QString title = "参数配置";
    if (isModified) {
        title += " *";
    }
    setWindowTitle(title);

    btnApply->setEnabled(isModified);
}
