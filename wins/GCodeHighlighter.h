#ifndef _GCODE_HIGHLIGHTER_
#define _GCODE_HIGHLIGHTER_

#include <QSyntaxHighlighter>
#include <QRegularExpression>

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

#endif//_GCODE_HIGHLIGHTER_
