#pragma once

#include "JavaVersion.h"
#include <QObject>

namespace x4 {
    class DownloadManager;

    class JavaProvisioner : public QObject {
        Q_OBJECT

    public:
        explicit JavaProvisioner(QObject *parent = nullptr);

        void provision(int majorVersion);

        signals:
    

        void progress(int percent, const QString &status);

        void finished(bool success, const JavaVersion &java, const QString &error);

    private:
        void onMetadataReceived(const QByteArray &data, int majorVersion);

        void extractArchive(const QString &archivePath, const QString &destDir, int majorVersion);

        static QString osName();

        static QString archName();
    };
}