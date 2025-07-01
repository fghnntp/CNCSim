#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include "tool_manager.h"

class ToolTableDialog : public QDialog {
    Q_OBJECT
public:
    ToolTableDialog(ToolManager *toolManger, QWidget* parent = nullptr);

private slots:
    void onAddTool();
    void onEditTool();
    void onSaveTool();
    void onRemoveTool();
    void refreshTable();

private:
    ToolManager* toolManager_;
    QTableWidget* tableWidget_;
    QPushButton* addButton_;
    QPushButton* editButton_;
    QPushButton* saveButton_;
    QPushButton* removeButton_;
};
