#pragma once
#include <QObject>
#include <QString>

namespace x4 {
    class GlobalEvents : public QObject {
        Q_OBJECT

    public:
        static GlobalEvents &instance() {
            static GlobalEvents ge;
            return ge;
        }

        signals:
    

        void downloadStarted(const QString &taskName);

        void downloadProgress(int percent, const QString &speedText);

        void downloadFinished(bool success, const QString &message);

        void stageProgress(const QString &stageName);

    private:
        GlobalEvents() = default;
    };
}