#include "CrashReportDialog.h"
#include "../core/Translator.h"
#include "../core/PathManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QClipboard>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QGroupBox>

namespace x4 {
    CrashReportDialog::CrashReportDialog(const ErrorDiagnosis &diagnosis, QWidget *parent)
        : QDialog(parent), diagnosis_(diagnosis) {
        setWindowFlag(Qt::Window, true);
        setWindowTitle(X4TR("crash.title"));
        setMinimumSize(500, 400);
        setupUI(diagnosis);
    }

    void CrashReportDialog::setupUI(const ErrorDiagnosis &diagnosis) {
        auto *layout = new QVBoxLayout(this);
        layout->setSpacing(16);
        layout->setContentsMargins(24, 24, 24, 24);

        // Header Area
        auto *headerLayout = new QHBoxLayout();

        auto *iconLabel = new QLabel(this);
        iconLabel->setText(getIconForCategory(diagnosis.category));
        QFont iconFont = iconLabel->font();
        iconFont.setPixelSize(32);
        iconLabel->setFont(iconFont);
        headerLayout->addWidget(iconLabel);

        auto *titleLabel = new QLabel(diagnosis.title, this);
        QFont titleFont = titleLabel->font();
        titleFont.setPixelSize(18);
        titleFont.setBold(true);
        titleLabel->setFont(titleFont);
        titleLabel->setStyleSheet(QStringLiteral("color: %1;").arg(getColorForCategory(diagnosis.category)));
        headerLayout->addWidget(titleLabel, 1);

        layout->addLayout(headerLayout);

        // Cause
        auto *causeLayout = new QVBoxLayout();
        auto *causeTitle = new QLabel(X4TR("crash.lbl.cause"), this);
        causeTitle->setStyleSheet(QStringLiteral("font-weight: bold; color: #a9b1d6;"));
        causeLayout->addWidget(causeTitle);

        auto *causeText = new QLabel(diagnosis.cause, this);
        causeText->setWordWrap(true);
        causeLayout->addWidget(causeText);
        layout->addLayout(causeLayout);

        // Actions
        if (!diagnosis.actions.isEmpty()) {
            auto *actionsGroup = new QGroupBox(X4TR("crash.lbl.actions"), this);
            auto *actionsLayout = new QVBoxLayout(actionsGroup);
            actionsLayout->setSpacing(8);
            for (int i = 0; i < diagnosis.actions.size(); ++i) {
                auto *actionLabel = new QLabel(QStringLiteral("• %1").arg(diagnosis.actions[i]), this);
                actionLabel->setWordWrap(true);
                actionsLayout->addWidget(actionLabel);
            }
            layout->addWidget(actionsGroup);
        }

        // Details (Expandable or just fixed for now)
        auto *detailsLayout = new QVBoxLayout();
        auto *detailsTitle = new QLabel(X4TR("crash.lbl.details"), this);
        detailsTitle->setStyleSheet(QStringLiteral("font-weight: bold; color: #a9b1d6;"));
        detailsLayout->addWidget(detailsTitle);

        auto *detailsEdit = new QTextEdit(this);
        detailsEdit->setPlainText(diagnosis.technicalDetail);
        detailsEdit->setReadOnly(true);
        detailsEdit->setMaximumHeight(100);
        detailsEdit->setStyleSheet(QStringLiteral("font-family: monospace; font-size: 11px; background: #111320;"));
        detailsLayout->addWidget(detailsEdit);
        layout->addLayout(detailsLayout);

        // Buttons
        auto *btnLayout = new QHBoxLayout();

        auto *copyBtn = new QPushButton(X4TR("crash.btn.copy"), this);
        connect(copyBtn, &QPushButton::clicked, this, [this]() {
            QApplication::clipboard()->setText(diagnosis_.technicalDetail);
        });
        btnLayout->addWidget(copyBtn);

        auto *logBtn = new QPushButton(X4TR("crash.btn.log"), this);
        connect(logBtn, &QPushButton::clicked, this, []() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(PathManager::instance().logsDir()));
        });
        btnLayout->addWidget(logBtn);

        btnLayout->addStretch();

        auto *closeBtn = new QPushButton(X4TR("crash.btn.close"), this);
        closeBtn->setDefault(true);
        connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
        btnLayout->addWidget(closeBtn);

        layout->addLayout(btnLayout);
    }

    QString CrashReportDialog::getIconForCategory(ErrorCategory cat) const {
        switch (cat) {
            case ErrorCategory::Java: return QStringLiteral("☕");
            case ErrorCategory::Network: return QStringLiteral("🌐");
            case ErrorCategory::FileSystem: return QStringLiteral("📁");
            case ErrorCategory::Launch: return QStringLiteral("💥");
            case ErrorCategory::ModLoader: return QStringLiteral("🧩");
            case ErrorCategory::Auth: return QStringLiteral("🔐");
            default: return QStringLiteral("⚠️");
        }
    }

    QString CrashReportDialog::getColorForCategory(ErrorCategory cat) const {
        switch (cat) {
            case ErrorCategory::Java: return QStringLiteral("#e2b74f"); // Warning yellow
            case ErrorCategory::Launch: return QStringLiteral("#f7768e"); // Error red
            case ErrorCategory::Network: return QStringLiteral("#7aa2f7"); // Info blue
            default: return QStringLiteral("#c0caf5"); // Default text
        }
    }
} // namespace x4