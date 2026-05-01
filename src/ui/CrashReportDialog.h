#pragma once

#include <QDialog>
#include "../core/ErrorHandler.h"

namespace x4 {
    class CrashReportDialog : public QDialog {
        Q_OBJECT

    public:
        explicit CrashReportDialog(const ErrorDiagnosis &diagnosis, QWidget *parent = nullptr);

    private:
        void setupUI(const ErrorDiagnosis &diagnosis);

        QString getIconForCategory(ErrorCategory cat) const;

        QString getColorForCategory(ErrorCategory cat) const;

        ErrorDiagnosis diagnosis_;
    };
} // namespace x4