#ifndef GCODEEDIT_H
#define GCODEEDIT_H

#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QRegularExpression>
#include <QTextCharFormat>
#include <QVector>

// G代码语法高亮器
class GCodeHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit GCodeHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;
};

// G代码编辑器
class GCodeEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit GCodeEdit(QWidget *parent = nullptr);
    ~GCodeEdit();

    // 公共接口
    bool loadFromFile(const QString &fileName);
    bool saveToFile(const QString &fileName);
    void setGCode(const QString &gcode);
    QString getGCode() const;
    
    // 设置选项
    void setShowLineNumbers(bool show);
    void setHighlightEnabled(bool enabled);
    QString getActiveFilePath();
private:
    QString currentFilePath;  // 当前文件路径
public:
    //save item
    QString getCurrentFilePath() const { return currentFilePath; }
    void setCurrentFilePath(const QString &path) { currentFilePath = path; }
    bool hasValidFilePath() const { return !currentFilePath.isEmpty();}

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    void setupEditor();
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

    class LineNumberArea;
    LineNumberArea *lineNumberArea;
    GCodeHighlighter *highlighter;
    bool showLineNumbers;
};

// 行号显示区域
class GCodeEdit::LineNumberArea : public QWidget
{
public:
    LineNumberArea(GCodeEdit *editor) : QWidget(editor), codeEditor(editor) {}

    QSize sizeHint() const override
    {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    GCodeEdit *codeEditor;
};

#endif // GCODEEDIT_H
