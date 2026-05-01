#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

namespace x4 {
    enum class ErrorCategory {
        Launch,
        Network,
        Java,
        FileSystem,
        ModLoader,
        Auth,
        Unknown
    };

    struct ErrorDiagnosis {
        ErrorCategory category;
        QString title;
        QString cause;
        QStringList actions;
        QString technicalDetail;
    };

    class ErrorHandler {
    public:
        static ErrorHandler &instance();

        void initDefaults();

        void registerPattern(const QString &pattern, ErrorCategory cat,
                             const QString &title, const QString &cause,
                             const QStringList &actions);

        ErrorDiagnosis diagnose(const QString &errorMessage) const;

    private:
        ErrorHandler() = default;

        struct Pattern {
            QString regex;
            ErrorCategory category;
            QString title;
            QString cause;
            QStringList actions;
        };

        QVector<Pattern> patterns_;
    };
} // namespace x4