#ifndef _CMD_TEXT_EDIT_H_
#define _CMD_TEXT_EDIT_H_

#include <QTextEdit>

class CommandTextEdit : public QTextEdit {
    Q_OBJECT
public:
    explicit CommandTextEdit(QWidget* parent = nullptr);

    void appendOutput(const QString& text);

signals:
    void commandEntered(const QString& cmd);
    void tabPressed();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QStringList commandHistory;
    int historyIndex = -1;
    QString currentEditingCommand; // 保存当前正在编辑的命令
    QString lastCommand;  // 只记录最后一条命令

    QString getCurrentLine() const;
    void replaceCurrentLine(const QString& text);
    QString getCurrentCommand() const;
    void ensureAtEditablePosition();
};

#endif
