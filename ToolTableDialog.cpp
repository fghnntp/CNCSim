#include "ToolTableDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>

ToolTableDialog::ToolTableDialog(ToolManager *toolManager, QWidget* parent)
    : QDialog(parent), toolManager_(toolManager)
{
    setWindowTitle("刀具表管理");
    resize(600, 400);


    tableWidget_ = new QTableWidget(this);
    tableWidget_->setColumnCount(5);
    tableWidget_->setHorizontalHeaderLabels({"刀具号", "刀库号", "长度补偿", "直径补偿", "描述"});
    tableWidget_->setSelectionBehavior(QAbstractItemView::SelectRows);

    addButton_ = new QPushButton("新增", this);
    editButton_ = new QPushButton("编辑", this);
    removeButton_ = new QPushButton("删除", this);

    QHBoxLayout* btnLayout = new QHBoxLayout;
    btnLayout->addWidget(addButton_);
    btnLayout->addWidget(editButton_);
    btnLayout->addWidget(removeButton_);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tableWidget_);
    mainLayout->addLayout(btnLayout);

    connect(addButton_, &QPushButton::clicked, this, &ToolTableDialog::onAddTool);
    connect(editButton_, &QPushButton::clicked, this, &ToolTableDialog::onEditTool);
    connect(removeButton_, &QPushButton::clicked, this, &ToolTableDialog::onRemoveTool);

    refreshTable();
}

void ToolTableDialog::refreshTable() {
    auto tools = toolManager_->get_all_tools();
    tableWidget_->setRowCount(tools.size());
    for (int i = 0; i < tools.size(); ++i) {
        const auto& t = tools[i];
        tableWidget_->setItem(i, 0, new QTableWidgetItem(QString::number(t.tool_number)));
        tableWidget_->setItem(i, 1, new QTableWidgetItem(QString::number(t.pocket_number)));
        tableWidget_->setItem(i, 2, new QTableWidgetItem(QString::number(t.length_offset)));
        tableWidget_->setItem(i, 3, new QTableWidgetItem(QString::number(t.diameter_offset)));
        tableWidget_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(t.description)));
    }
}

void ToolTableDialog::onAddTool() {
    // 简单输入对话框，实际可做成自定义表单
    bool ok;
    int tno = QInputDialog::getInt(this, "新增刀具", "刀具号：", 1, 1, 9999, 1, &ok);
    if (!ok) return;
    int pno = QInputDialog::getInt(this, "新增刀具", "刀库号：", 1, 1, 9999, 1, &ok);
    if (!ok) return;
    double z = QInputDialog::getDouble(this, "新增刀具", "长度补偿：", 0, -1000, 1000, 2, &ok);
    if (!ok) return;
    double d = QInputDialog::getDouble(this, "新增刀具", "直径补偿：", 0, -1000, 1000, 2, &ok);
    if (!ok) return;
    QString desc = QInputDialog::getText(this, "新增刀具", "描述：", QLineEdit::Normal, "", &ok);
    if (!ok) return;

    ToolData tool(tno, pno, z, d, desc.toStdString());
    if (!toolManager_->add_tool(tool)) {
        QMessageBox::warning(this, "错误", "添加刀具失败！");
    }
    refreshTable();
}

void ToolTableDialog::onEditTool() {
    int row = tableWidget_->currentRow();
    if (row < 0) return;
    int tno = tableWidget_->item(row, 0)->text().toInt();
    const ToolData* tool = toolManager_->get_tool(tno);
    if (!tool) return;

    bool ok;
    int pno = QInputDialog::getInt(this, "编辑刀具", "刀库号：", tool->pocket_number, 1, 9999, 1, &ok);
    if (!ok) return;
    double z = QInputDialog::getDouble(this, "编辑刀具", "长度补偿：", tool->length_offset, -1000, 1000, 2, &ok);
    if (!ok) return;
    double d = QInputDialog::getDouble(this, "编辑刀具", "直径补偿：", tool->diameter_offset, -1000, 1000, 2, &ok);
    if (!ok) return;
    QString desc = QInputDialog::getText(this, "编辑刀具", "描述：", QLineEdit::Normal, QString::fromStdString(tool->description), &ok);
    if (!ok) return;

    ToolData newTool(tno, pno, z, d, desc.toStdString());
    toolManager_->update_tool(tno, newTool);
    refreshTable();
}

void ToolTableDialog::onRemoveTool() {
    int row = tableWidget_->currentRow();
    if (row < 0) return;
    int tno = tableWidget_->item(row, 0)->text().toInt();
    if (QMessageBox::question(this, "确认", "确定要删除该刀具吗？") == QMessageBox::Yes) {
        toolManager_->remove_tool(tno);
        refreshTable();
    }
}
