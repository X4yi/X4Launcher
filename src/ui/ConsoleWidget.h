#pragma once

#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTimer>

class QLineEdit;

namespace x4 {


class ConsoleHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit ConsoleHighlighter(QTextDocument* parent);

protected:
    void highlightBlock(const QString& text) override;

private:
    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<Rule> rules_;
};



class ConsoleWidget : public QWidget {
    Q_OBJECT
public:
    explicit ConsoleWidget(QWidget* parent = nullptr);

    
    void appendLine(const QString& line);

    
    void clear();

    
    QString allText() const;

public slots:
    void toggleSearch();

private:
    void flushBatch();
    void setupUI();
    void onSearch(const QString& text);

    QPlainTextEdit* textEdit_;
    ConsoleHighlighter* highlighter_;
    QWidget* searchBar_;
    QLineEdit* searchInput_;
    QTimer batchTimer_;
    QStringList pendingLines_;
    int maxLines_ = 10000;
};

} 
