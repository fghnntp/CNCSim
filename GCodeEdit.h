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

    // 搜索功能
    bool find(const QString &searchText,
             QTextDocument::FindFlags flags = QTextDocument::FindFlags());
    bool findNext();
    bool findPrevious();
    void clearSearchHighlight();

    // 替换功能
    void replace(const QString &searchText, const QString &replaceText,
                QTextDocument::FindFlags flags = QTextDocument::FindFlags());
    void replaceAll(const QString &searchText, const QString &replaceText,
                   QTextDocument::FindFlags flags = QTextDocument::FindFlags());
private:
    QString currentFilePath;  // 当前文件路径
public:
    //save item
    QString getCurrentFilePath() const { return currentFilePath; }
    void setCurrentFilePath(const QString &path) { currentFilePath = path; }
    bool hasValidFilePath() const { return !currentFilePath.isEmpty();}

signals:
    void searchResult(bool found);

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
    void highlightSearchResults(const QString &searchText);

    class LineNumberArea;
    LineNumberArea *lineNumberArea;
    GCodeHighlighter *highlighter;
    bool showLineNumbers;

    // Search related members
    QTextCursor lastFoundCursor;

private:
    QString lastSearchText;
    QList<QTextEdit::ExtraSelection> searchSelections;
    QList<QTextEdit::ExtraSelection> otherSelections; // For non-search highlights
    void updateExtraSelections();
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
