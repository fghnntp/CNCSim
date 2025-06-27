#include "GCodeEdit.h"
#include <QTextStream>
#include <QFile>
#include <QFont>
#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>

// GCodeHighlighter 实现
GCodeHighlighter::GCodeHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    // G代码指令 (G00, G01等)
    QTextCharFormat gCodeFormat;
    gCodeFormat.setForeground(QColor(0, 100, 200));
    gCodeFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("\\bG\\d+");
    rule.format = gCodeFormat;
    highlightingRules.append(rule);

    // M代码指令 (M03, M05等)
    QTextCharFormat mCodeFormat;
    mCodeFormat.setForeground(QColor(200, 0, 100));
    mCodeFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("\\bM\\d+");
    rule.format = mCodeFormat;
    highlightingRules.append(rule);

    // T代码指令 (工具)
    QTextCharFormat tCodeFormat;
    tCodeFormat.setForeground(QColor(150, 0, 150));
    tCodeFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("\\bT\\d+");
    rule.format = tCodeFormat;
    highlightingRules.append(rule);

    // 坐标轴 (X, Y, Z等)
    QTextCharFormat axisFormat;
    axisFormat.setForeground(QColor(0, 150, 0));
    axisFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("\\b[XYZIJKFSR][-+]?\\d*\\.?\\d+");
    rule.format = axisFormat;
    highlightingRules.append(rule);

    // 数字
    QTextCharFormat numberFormat;
    numberFormat.setForeground(QColor(200, 100, 0));
    rule.pattern = QRegularExpression("\\b\\d+\\.?\\d*");
    rule.format = numberFormat;
    highlightingRules.append(rule);

    // 注释 - 括号注释
    QTextCharFormat commentFormat;
    commentFormat.setForeground(QColor(128, 128, 128));
    commentFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("\\([^)]*\\)");
    rule.format = commentFormat;
    highlightingRules.append(rule);

    // 注释 - 分号注释
    QTextCharFormat semicolonCommentFormat;
    semicolonCommentFormat.setForeground(QColor(128, 128, 128));
    semicolonCommentFormat.setFontItalic(true);
    rule.pattern = QRegularExpression(";.*$");
    rule.format = semicolonCommentFormat;
    highlightingRules.append(rule);
}

void GCodeHighlighter::highlightBlock(const QString &text)
{
    foreach (const HighlightingRule &rule, highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

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
    // QFile file(fileName);
    // if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    //     return false;
    // }

    // QTextStream in(&file);
    // setPlainText(in.readAll());
    // return true;
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
    // QFile file(fileName);
    // if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    //     return false;
    // }

    // QTextStream out(&file);
    // out << toPlainText();
    // return true;
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
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(Qt::yellow).lighter(160);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
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
