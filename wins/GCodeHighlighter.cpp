#include "GCodeHighlighter.h"

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
