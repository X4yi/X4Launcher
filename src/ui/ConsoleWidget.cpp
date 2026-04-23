#include "ConsoleWidget.h"
#include "Theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextBlock>
#include <QShortcut>
#include <QRegularExpression>
#include <QScrollBar>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QApplication>

namespace x4 {



ConsoleHighlighter::ConsoleHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    
    QTextCharFormat errorFmt;
    errorFmt.setForeground(Theme::Colors::logError());
    errorFmt.setFontWeight(QFont::Bold);
    Rule errorRule;
    errorRule.pattern = QRegularExpression(QStringLiteral(
        R"(.*(?:ERROR|FATAL|Exception|java\.lang\.\w*Exception|Caused by:|at\s+\w+).*)" ));
    errorRule.format = errorFmt;
    rules_.push_back(std::move(errorRule));

    
    QTextCharFormat warnFmt;
    warnFmt.setForeground(Theme::Colors::logWarn());
    Rule warnRule;
    warnRule.pattern = QRegularExpression(QStringLiteral(R"(.*\bWARN\b.*)"));
    warnRule.format = warnFmt;
    rules_.push_back(std::move(warnRule));

    
    QTextCharFormat infoFmt;
    infoFmt.setForeground(Theme::Colors::logInfo());
    Rule infoRule;
    infoRule.pattern = QRegularExpression(QStringLiteral(R"(.*\bINFO\b.*)"));
    infoRule.format = infoFmt;
    rules_.push_back(std::move(infoRule));

    
    QTextCharFormat timeFmt;
    timeFmt.setForeground(Theme::Colors::logTime());
    Rule timeRule;
    timeRule.pattern = QRegularExpression(QStringLiteral(R"(\[\d{2}:\d{2}:\d{2}\])"));
    timeRule.format = timeFmt;
    rules_.push_back(std::move(timeRule));

    
    QTextCharFormat debugFmt;
    debugFmt.setForeground(Theme::Colors::logDebug());
    Rule debugRule;
    debugRule.pattern = QRegularExpression(QStringLiteral(R"(.*\bDEBUG\b.*)"));
    debugRule.format = debugFmt;
    rules_.push_back(std::move(debugRule));
}

void ConsoleHighlighter::highlightBlock(const QString& text) {
    for (const auto& rule : rules_) {
        auto it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            auto match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}



ConsoleWidget::ConsoleWidget(QWidget* parent) : QWidget(parent) {
    setupUI();

    
    batchTimer_.setInterval(16);
    batchTimer_.setSingleShot(true);
    connect(&batchTimer_, &QTimer::timeout, this, &ConsoleWidget::flushBatch);
}

void ConsoleWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    
    searchBar_ = new QWidget(this);
    searchBar_->setVisible(false);
    auto* searchLayout = new QHBoxLayout(searchBar_);
    searchLayout->setContentsMargins(8, 4, 8, 4);

    searchInput_ = new QLineEdit(searchBar_);
    searchInput_->setPlaceholderText(QStringLiteral("Search logs..."));
    searchInput_->setClearButtonEnabled(true);
    connect(searchInput_, &QLineEdit::textChanged, this, &ConsoleWidget::onSearch);

    auto* closeSearch = new QPushButton(QStringLiteral("✕"), searchBar_);
    closeSearch->setFixedSize(24, 24);
    closeSearch->setFlat(true);
    connect(closeSearch, &QPushButton::clicked, this, [this]() {
        searchBar_->setVisible(false);
        searchInput_->clear();
    });

    searchLayout->addWidget(searchInput_);
    searchLayout->addWidget(closeSearch);
    layout->addWidget(searchBar_);

    
    textEdit_ = new QPlainTextEdit(this);
    textEdit_->setReadOnly(true);
    textEdit_->setMaximumBlockCount(maxLines_);
    textEdit_->setWordWrapMode(QTextOption::NoWrap);
    textEdit_->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);

    
    textEdit_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(textEdit_, &QPlainTextEdit::customContextMenuRequested, this,
            [this](const QPoint& pos) {
                auto* menu = new QMenu(this);
                auto* copyAct = menu->addAction(QStringLiteral("Copy"));
                copyAct->setShortcut(QKeySequence::Copy);
                connect(copyAct, &QAction::triggered, textEdit_, &QPlainTextEdit::copy);

                auto* selAllAct = menu->addAction(QStringLiteral("Select All"));
                selAllAct->setShortcut(QKeySequence::SelectAll);
                connect(selAllAct, &QAction::triggered, textEdit_, &QPlainTextEdit::selectAll);

                menu->addSeparator();
                auto* searchAct = menu->addAction(QStringLiteral("Search"));
                searchAct->setShortcut(QKeySequence::Find);
                connect(searchAct, &QAction::triggered, this, &ConsoleWidget::toggleSearch);

                menu->addSeparator();
                auto* clearAct = menu->addAction(QStringLiteral("Clear"));
                connect(clearAct, &QAction::triggered, this, &ConsoleWidget::clear);

                menu->exec(textEdit_->mapToGlobal(pos));
                menu->deleteLater();
            });

    highlighter_ = new ConsoleHighlighter(textEdit_->document());
    layout->addWidget(textEdit_);

    
    auto* findShortcut = new QShortcut(QKeySequence::Find, this);
    connect(findShortcut, &QShortcut::activated, this, &ConsoleWidget::toggleSearch);
}

void ConsoleWidget::appendLine(const QString& line) {
    pendingLines_.append(line);
    if (!batchTimer_.isActive()) {
        batchTimer_.start();
    }
}

void ConsoleWidget::flushBatch() {
    if (pendingLines_.isEmpty()) return;

    
    auto* sb = textEdit_->verticalScrollBar();
    bool atBottom = sb->value() >= sb->maximum() - 4;

    
    QString batch = pendingLines_.join('\n');
    pendingLines_.clear();

    textEdit_->appendPlainText(batch);

    
    if (atBottom) {
        sb->setValue(sb->maximum());
    }
}

void ConsoleWidget::clear() {
    pendingLines_.clear();
    textEdit_->clear();
}

QString ConsoleWidget::allText() const {
    return textEdit_->toPlainText();
}

void ConsoleWidget::toggleSearch() {
    bool vis = !searchBar_->isVisible();
    searchBar_->setVisible(vis);
    if (vis) {
        searchInput_->setFocus();
        searchInput_->selectAll();
    }
}

void ConsoleWidget::onSearch(const QString& text) {
    if (text.isEmpty()) {
        
        auto cursor = textEdit_->textCursor();
        cursor.clearSelection();
        textEdit_->setTextCursor(cursor);
        return;
    }

    
    auto flags = QTextDocument::FindFlags();
    if (!textEdit_->find(text, flags)) {
        
        auto cursor = textEdit_->textCursor();
        cursor.movePosition(QTextCursor::Start);
        textEdit_->setTextCursor(cursor);
        textEdit_->find(text, flags);
    }
}

} 
