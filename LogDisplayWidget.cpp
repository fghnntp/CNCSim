#include "LogDisplayWidget.h"
#include <QMenu>
#include <QFile>
#include <QTextStream>
#include <QScrollBar>
#include <QClipboard>
#include <QApplication>
#include <QRegularExpression>

// ==================== LogHighlighter ====================

LogHighlighter::LogHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent) {
    setupRules();
}

void LogHighlighter::setupRules() {
    // 错误日志 (红色)
    HighlightRule errorRule;
    errorRule.pattern = QRegularExpression(R"(^\[ERROR\].*|error|fail)",
                                         QRegularExpression::CaseInsensitiveOption);
    errorRule.format.setForeground(Qt::red);
    errorRule.format.setFontWeight(QFont::Bold);
    highlightingRules.append(errorRule);

    // 警告日志 (黄色)
    HighlightRule warningRule;
    warningRule.pattern = QRegularExpression(R"(^\[WARN\].*|warn|alert)",
                                           QRegularExpression::CaseInsensitiveOption);
    warningRule.format.setForeground(QColor(200, 120, 0)); // 橙色
    warningRule.format.setFontWeight(QFont::Bold);
    highlightingRules.append(warningRule);

    // 成功日志 (绿色)
    HighlightRule successRule;
    successRule.pattern = QRegularExpression(R"(^\[SUCCESS\].*|success|ok|done)",
                                           QRegularExpression::CaseInsensitiveOption);
    successRule.format.setForeground(Qt::darkGreen);
    successRule.format.setFontWeight(QFont::Bold);
    highlightingRules.append(successRule);

    // 信息日志 (蓝色)
    HighlightRule infoRule;
    infoRule.pattern = QRegularExpression(R"(^\[INFO\].*|info|notice)",
                                         QRegularExpression::CaseInsensitiveOption);
    infoRule.format.setForeground(Qt::blue);
    highlightingRules.append(infoRule);

    // 时间戳 (灰色)
    HighlightRule timestampRule;
    timestampRule.pattern = QRegularExpression(R"(\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\])");
    timestampRule.format.setForeground(Qt::darkGray);
    highlightingRules.append(timestampRule);
}

void LogHighlighter::highlightBlock(const QString &text) {
    for (const HighlightRule &rule : highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

// ==================== LogDisplayWidget ====================

LogDisplayWidget::LogDisplayWidget(QWidget *parent)
    : QTextEdit(parent) {
    // 基本设置
    setReadOnly(true);
    setLineWrapMode(QTextEdit::WidgetWidth);
    setWordWrapMode(QTextOption::WrapAnywhere);
    setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);

    // 设置字体
    QFont font("Consolas", 10);
    setFont(font);

    // 初始化高亮器
    highlighter = new LogHighlighter(document());

    // 设置上下文菜单
    setupContextMenu();
}

void LogDisplayWidget::appendLog(const QString &log, const QColor &color) {
    // 保存当前滚动条位置
    bool atBottom = verticalScrollBar()->value() == verticalScrollBar()->maximum();

    // 设置文本颜色并追加日志
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);

    QTextCharFormat format;
    format.setForeground(color);
    cursor.setCharFormat(format);

    cursor.insertText(log + "\n");

    // 如果之前已经在底部，保持滚动到底部
    if (atBottom) {
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    }

    // 修剪超出行数
    trimExcessLines();

    // 发出信号
    emit logAdded(log);
}

void LogDisplayWidget::appendError(const QString &error) {
    appendLog("[ERROR] " + error, Qt::red);
    emit errorAdded(error);
}

void LogDisplayWidget::appendWarning(const QString &warning) {
    appendLog("[WARN] " + warning, QColor(200, 120, 0)); // 橙色
    emit warningAdded(warning);
}

void LogDisplayWidget::appendSuccess(const QString &success) {
    appendLog("[SUCCESS] " + success, Qt::darkGreen);
}

void LogDisplayWidget::appendInfo(const QString &info) {
    appendLog("[INFO] " + info, Qt::blue);
}

void LogDisplayWidget::clearLogs() {
    clear();
}

void LogDisplayWidget::saveToFile(const QString &filePath) {
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << toPlainText();
        file.close();
    }
}

void LogDisplayWidget::setMaxLines(int max) {
    m_maxLines = max;
    trimExcessLines();
}

int LogDisplayWidget::maxLines() const {
    return m_maxLines;
}

void LogDisplayWidget::trimExcessLines() {
    if (m_maxLines <= 0) return;

    QTextCursor cursor(document());
    int lineCount = document()->blockCount();

    while (lineCount > m_maxLines) {
        cursor.movePosition(QTextCursor::Start);
        cursor.select(QTextCursor::BlockUnderCursor);
        cursor.removeSelectedText();
        cursor.deleteChar(); // 删除换行符
        lineCount--;
    }
}

void LogDisplayWidget::setupContextMenu() {
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &LogDisplayWidget::customContextMenuRequested,
            this, [this](const QPoint &pos) {
        QMenu *menu = createStandardContextMenu();

        menu->addSeparator();
        menu->addAction("Clear All", this, &LogDisplayWidget::clearLogs);
        menu->addAction("Copy All", [this]() {
            QApplication::clipboard()->setText(toPlainText());
        });

        emit contextMenuRequested(pos);
        menu->exec(mapToGlobal(pos));
        delete menu;
    });
}

void LogDisplayWidget::contextMenuEvent(QContextMenuEvent *event) {
    // 事件已通过 customContextMenuRequested 信号处理
    event->accept();
}
