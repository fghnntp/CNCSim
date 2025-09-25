#ifndef GCODEEDIT_H
#define GCODEEDIT_H

#include <QPlainTextEdit>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QVector>
#include "GCodeHighlighter.h"


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


public:
    // 1-based 行号
    void highlightLineOne(long long line);
    void highlightLine(int line, const QColor& color = QColor(255,255,0,80),
                       bool centerView = true);

    void highlightLines(int fromLine, int toLine,
                        const QColor& color = QColor(255,255,0,80),
                        bool centerView = true);
    void clearLineHighlight(int line);    // 取消某一行的高亮
    void clearAllLineHighlights();        // 取消全部高亮
    void flashHighlightLine(int line, int msec = 800,
                            const QColor& color = QColor(255,255,0,110));

private:
    QList<QTextEdit::ExtraSelection> lineHighlights;
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
