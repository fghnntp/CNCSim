#include "CmdTextEdit.h"
#include <QKeyEvent>
#include <QTextBlock>
#include <QDebug>

CommandTextEdit::CommandTextEdit(QWidget* parent) : QTextEdit(parent) {
    setLineWrapMode(QTextEdit::WidgetWidth);
    setStyleSheet("font-family: Consolas, 'Courier New', monospace;");
    append("> ");
}

void CommandTextEdit::appendOutput(const QString& text) {
    moveCursor(QTextCursor::End);
    insertPlainText("\n" + text);
    append("> ");
}

void CommandTextEdit::keyPressEvent(QKeyEvent* event) {
    ensureAtEditablePosition();

    switch (event->key()) {
    case Qt::Key_Up:
        if (!commandHistory.isEmpty()) {
            if (historyIndex < commandHistory.size() - 1) {
                if (historyIndex == -1) {
                    // 首次按上键，保存当前未执行的命令
                    currentEditingCommand = getCurrentLine().mid(2); // 去掉提示符"> "
                }
                historyIndex++;
                replaceCurrentLine(commandHistory.at(historyIndex));
            }
        }
        event->accept();
        break;

    case Qt::Key_Down:
        if (historyIndex > 0) {
            historyIndex--;
            replaceCurrentLine(commandHistory.at(historyIndex));
        }
        else if (historyIndex == 0) {
            historyIndex = -1;
            // 恢复到之前正在编辑的命令
            replaceCurrentLine(currentEditingCommand);
        }
        event->accept();
        break;

    case Qt::Key_Return:
        if (!(event->modifiers() & Qt::ShiftModifier)) {
            QString cmd = getCurrentCommand();
            if (!cmd.isEmpty()) {
                commandHistory.prepend(cmd);
                historyIndex = -1;
                emit commandEntered(cmd);
                append("\n> ");
            }
            event->accept();
            return;
        }
        break;

    case Qt::Key_Tab:
        emit tabPressed();
        event->accept();
        return;

    case Qt::Key_Backspace:
    case Qt::Key_Delete:
        if (textCursor().positionInBlock() <= 2) {
            event->ignore();
            return;
        }
        break;

    default:
        break;
    }

    QTextEdit::keyPressEvent(event);
}

void CommandTextEdit::mousePressEvent(QMouseEvent* event) {
    ensureAtEditablePosition();
    QTextEdit::mousePressEvent(event);
}

void CommandTextEdit::mouseMoveEvent(QMouseEvent* event) {
    ensureAtEditablePosition();
    QTextEdit::mouseMoveEvent(event);
}

QString CommandTextEdit::getCurrentLine() const {
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    return cursor.selectedText();
}

void CommandTextEdit::replaceCurrentLine(const QString& text) {
    QTextCursor cursor = textCursor();

    // 保存原始光标位置（在行中的偏移量）
    int inBlockPosition = cursor.positionInBlock();

    // 选中当前行内容（不包括行尾换行符）
    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);

    // 检查是否是带提示符的行
    bool hasPrompt = cursor.selectedText().startsWith("> ");

    // 替换内容
    cursor.removeSelectedText();
    cursor.insertText(hasPrompt ? "> " + text : text);

    // 恢复光标位置
    if (inBlockPosition <= 2 && hasPrompt) {
       // 如果原来在提示符后，保持在提示符后
       cursor.movePosition(QTextCursor::StartOfLine);
       cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, 2);
    } else {
       // 否则尽量保持原始位置
       cursor.movePosition(QTextCursor::StartOfLine);
       cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor,
                          qMin(inBlockPosition, cursor.block().length() - 1));
    }

   setTextCursor(cursor);
}

QString CommandTextEdit::getCurrentCommand() const {
    // 获取当前光标所在行的文本
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);

    QString lineText = cursor.selectedText();

    // 去掉提示符 "> " 并去除首尾空格
    if (lineText.startsWith("> ")) {
        return lineText.mid(2).trimmed();
    }

    return lineText.trimmed();
}

void CommandTextEdit::ensureAtEditablePosition() {
    QTextCursor cursor = textCursor();
    if (cursor.blockNumber() < document()->blockCount() - 1) {
        cursor.movePosition(QTextCursor::End);
        setTextCursor(cursor);
    }
}
