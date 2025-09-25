#include "GCodeEdit.h"
#include "qdebug.h"
#include <QTextStream>
#include <QFile>
#include <QFont>
#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QMessageBox>
#include <QTimer>



// GCodeEdit 实现
GCodeEdit::GCodeEdit(QWidget *parent)
    : QPlainTextEdit(parent)
    , lineNumberArea(nullptr)
    , highlighter(nullptr)
    , showLineNumbers(true)
    , currentFilePath("")
{
    setupEditor();
}

GCodeEdit::~GCodeEdit()
{
}

void GCodeEdit::setupEditor()
{
    // 设置字体
    QFont font("Consolas", 10);
    if (!font.exactMatch()) {
        font.setFamily("Courier New");
    }
    font.setFixedPitch(true);
    setFont(font);

    // 设置行为
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setTabStopDistance(40); // 设置Tab宽度

    // 创建行号区域
    lineNumberArea = new LineNumberArea(this);

    // 连接信号
    connect(this, &GCodeEdit::blockCountChanged, this, &GCodeEdit::updateLineNumberAreaWidth);
    connect(this, &GCodeEdit::updateRequest, this, &GCodeEdit::updateLineNumberArea);
    connect(this, &GCodeEdit::cursorPositionChanged, this, &GCodeEdit::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    // 创建语法高亮
    highlighter = new GCodeHighlighter(document());
}

bool GCodeEdit::loadFromFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream in(&file);
    setPlainText(in.readAll());
    currentFilePath = fileName;  // 记录文件路径
    setWindowFilePath(fileName); // 设置窗口文件路径
    return true;
}

bool GCodeEdit::saveToFile(const QString &fileName)
{
     QString saveFileName = fileName;
    if (saveFileName.isEmpty()) {
        saveFileName = currentFilePath;
    }
    
    if (saveFileName.isEmpty()) {
        return false; // 没有文件名无法保存
    }

    QFile file(saveFileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << toPlainText();
    
    currentFilePath = saveFileName; // 更新当前文件路径
    setWindowFilePath(saveFileName);
    return true;
}

void GCodeEdit::setGCode(const QString &gcode)
{
    setPlainText(gcode);
}

QString GCodeEdit::getGCode() const
{
    return toPlainText();
}

void GCodeEdit::setShowLineNumbers(bool show)
{
    showLineNumbers = show;
    lineNumberArea->setVisible(show);
    updateLineNumberAreaWidth(0);
}

void GCodeEdit::setHighlightEnabled(bool enabled)
{
    if (enabled && !highlighter) {
        highlighter = new GCodeHighlighter(document());
    } else if (!enabled && highlighter) {
        delete highlighter;
        highlighter = nullptr;
    }
}

QString GCodeEdit::getActiveFilePath()
{
    return currentFilePath;
}

int GCodeEdit::lineNumberAreaWidth()
{
    if (!showLineNumbers) {
        return 0;
    }

    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void GCodeEdit::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void GCodeEdit::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void GCodeEdit::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void GCodeEdit::highlightCurrentLine()
{
    if (!isReadOnly()) {
        otherSelections.clear();

        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(Qt::yellow).lighter(160);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();

        otherSelections.append(selection);
        updateExtraSelections();
    }

    int line = textCursor().blockNumber() + 1;
    int col  = textCursor().positionInBlock() + 1;
}

void GCodeEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    if (!showLineNumbers) {
        return;
    }

    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor(240, 240, 240));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                           Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

// Search functionality implementation
bool GCodeEdit::find(const QString &searchText, QTextDocument::FindFlags flags)
{
    if (searchText.isEmpty()) {
        clearSearchHighlight();
        emit searchResult(false);
        return false;
    }

    bool found = false;
    QTextCursor cursor = textCursor();

    if (lastSearchText != searchText) {
        // New search - start from current position
        cursor.setPosition(cursor.selectionStart());
        lastSearchText = searchText;
    }

    cursor = document()->find(searchText, cursor, flags);

    if (!cursor.isNull()) {
        setTextCursor(cursor);
        found = true;
    } else {
        // Wrap around if not found
        if (flags & QTextDocument::FindBackward) {
            cursor.movePosition(QTextCursor::End);
        } else {
            cursor.movePosition(QTextCursor::Start);
        }
        cursor = document()->find(searchText, cursor, flags);
        if (!cursor.isNull()) {
            setTextCursor(cursor);
            found = true;
        }
    }

    highlightSearchResults(searchText);
    emit searchResult(found);
    return found;
}

bool GCodeEdit::findNext()
{
    if (lastSearchText.isEmpty()) return false;
    return find(lastSearchText);
}

bool GCodeEdit::findPrevious()
{
    if (lastSearchText.isEmpty()) return false;
    return find(lastSearchText, QTextDocument::FindBackward);
}

void GCodeEdit::clearSearchHighlight()
{
    searchSelections.clear();
    updateExtraSelections();
    lastSearchText.clear();
}

void GCodeEdit::updateExtraSelections()
{
    QList<QTextEdit::ExtraSelection> allSelections;
    allSelections.append(otherSelections);   // 当前行
    allSelections.append(searchSelections);  // 搜索结果
    allSelections.append(lineHighlights);    // 指定行
    setExtraSelections(allSelections);
}

void GCodeEdit::highlightSearchResults(const QString &searchText)
{
    searchSelections.clear();

    if (searchText.isEmpty()) {
        updateExtraSelections();
        return;
    }

    QTextEdit::ExtraSelection selection;
    QColor highlightColor = QColor(Qt::yellow).lighter(160);
    selection.format.setBackground(highlightColor);

    QTextCursor cursor(document());
    while (!cursor.isNull() && !cursor.atEnd()) {
        cursor = document()->find(searchText, cursor);
        if (!cursor.isNull()) {
            selection.cursor = cursor;
            searchSelections.append(selection);
        }
    }

    updateExtraSelections();
}

void GCodeEdit::replace(const QString &searchText, const QString &replaceText,
                       QTextDocument::FindFlags flags)
{
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection() && cursor.selectedText() == searchText) {
        cursor.insertText(replaceText);
    }
    find(searchText, flags);
}

void GCodeEdit::replaceAll(const QString &searchText, const QString &replaceText,
                         QTextDocument::FindFlags flags)
{
    QTextCursor cursor(document());
    cursor.beginEditBlock();

    int count = 0;
    while (true) {
        cursor = document()->find(searchText, cursor, flags);
        if (cursor.isNull()) break;

        cursor.insertText(replaceText);
        count++;
    }

    cursor.endEditBlock();

    if (count > 0) {
        QMessageBox::information(this, tr("Replace All"),
                               tr("Replaced %1 occurrences").arg(count));
    } else {
        QMessageBox::information(this, tr("Replace All"),
                               tr("No occurrences found"));
    }
}






static inline QTextBlock blockByOneBasedLine(const QPlainTextEdit* edit, int line1)
{
    int idx0 = qBound(1, line1, edit->blockCount()) - 1; // 安全到 0-based
    return edit->document()->findBlockByNumber(idx0);
}


void GCodeEdit::highlightLineOne(long long line)
{
    // 1-based -> clamp
    const int total = blockCount();
    if (total <= 0) return;

    clearAllLineHighlights();

    const long long clamped = std::max(1LL, std::min(line, static_cast<long long>(total)));
    const int oneBased = static_cast<int>(clamped);

    highlightLine(oneBased, QColor(255,255,0,80), true); // 调用三参版本
}


void GCodeEdit::highlightLine(int line, const QColor& color, bool centerView)
{
    QTextBlock block = blockByOneBasedLine(this, line);
    if (!block.isValid()) return;

    // 生成一条“整行高亮”的 selection
    QTextEdit::ExtraSelection sel;
    sel.format.setBackground(color);
    sel.format.setProperty(QTextFormat::FullWidthSelection, true);
    sel.cursor = QTextCursor(block);  // 光标指向该 block（不需要选择）

    // 若已存在同一行的高亮，先移除，避免重复
    for (int i = lineHighlights.size()-1; i >= 0; --i) {
        if (lineHighlights[i].cursor.blockNumber() == block.blockNumber())
            lineHighlights.removeAt(i);
    }
    lineHighlights.append(sel);



    if (centerView) {
        const QTextCursor old = textCursor();
        QTextCursor tmp(block);
        setTextCursor(tmp);
        centerCursor();               // 或 ensureCursorVisible();
        // setTextCursor(old);
    }

     updateExtraSelections();

}

void GCodeEdit::highlightLines(int fromLine, int toLine,
                               const QColor& color, bool centerView)
{
    if (fromLine > toLine) std::swap(fromLine, toLine);
    bool first = true;
    for (int ln = fromLine; ln <= toLine; ++ln) {
        highlightLine(ln, color, centerView && first);
        first = false;
    }
}

void GCodeEdit::clearLineHighlight(int line)
{
    QTextBlock block = blockByOneBasedLine(this, line);
    if (!block.isValid()) return;

    for (int i = lineHighlights.size()-1; i >= 0; --i) {
        if (lineHighlights[i].cursor.blockNumber() == block.blockNumber())
            lineHighlights.removeAt(i);
    }
    updateExtraSelections();
}

void GCodeEdit::clearAllLineHighlights()
{
    lineHighlights.clear();
    updateExtraSelections();
}

void GCodeEdit::flashHighlightLine(int line, int msec, const QColor& color)
{
    highlightLine(line, color, /*centerView=*/true);
    // 定时取消
    QTimer::singleShot(msec, this, [this, line]{
        clearLineHighlight(line);
    });
}


