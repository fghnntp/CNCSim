#ifndef _LOG_DISPLAY_WIDGET_
#define _LOG_DISPLAY_WIDGET_

#include <QTextEdit>
#include <QSyntaxHighlighter>
#include <QRegularExpression>

// 日志语法高亮器
class LogHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit LogHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightRule> highlightingRules;

    void setupRules();
};

// 日志显示部件
class LogDisplayWidget : public QTextEdit {
    Q_OBJECT
public:
    explicit LogDisplayWidget(QWidget *parent = nullptr);

    // 日志操作接口
    void appendLog(const QString &log, const QColor &color = Qt::black);
    void appendError(const QString &error);
    void appendWarning(const QString &warning);
    void appendSuccess(const QString &success);
    void appendInfo(const QString &info);

    void clearLogs();
    void saveToFile(const QString &filePath);

    // 设置最大行数 (防止内存占用过大)
    void setMaxLines(int max);
    int maxLines() const;

signals:
    void logAdded(const QString &log);
    void errorAdded(const QString &error);
    void warningAdded(const QString &warning);
    void contextMenuRequested(const QPoint &pos);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    LogHighlighter *highlighter;
    int m_maxLines = 10000; // 默认最大行数

    void trimExcessLines();
    void setupContextMenu();
};
#endif
