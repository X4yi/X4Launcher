#pragma once

#include "VersionManifest.h"
#include "VersionParser.h"
#include "ArgumentResolver.h"
#include "../instance/Instance.h"
#include "../instance/InstanceConfig.h"
#include "../cache/CacheManager.h"
#include "../java/JavaManager.h"
#include "../network/DownloadManager.h"
#include <QObject>
#include <QProcess>
#include <QDateTime>

namespace x4 {
    class InstanceManager;

    class LaunchEngine : public QObject {
        Q_OBJECT

    public:
        LaunchEngine(InstanceManager *instanceMgr, CacheManager *cacheMgr,
                     JavaManager *javaMgr, VersionManifest *manifest,
                     QObject *parent = nullptr);

        void launch(const QString &instanceId);

        bool isRunning(const QString &instanceId) const;

        void kill(const QString &instanceId);

        signals:
    

        void launchProgress(const QString &instanceId, const QString &status);

        void launchStarted(const QString &instanceId);

        void launchFailed(const QString &instanceId, const QString &error);

        void gameOutput(const QString &instanceId, const QString &line);

        void gameFinished(const QString &instanceId, int exitCode);

    private:
        void onVersionJsonReady(const QString &instanceId, const QByteArray &data);

        void fetchLoaderMeta(const QString &instanceId, const QByteArray &vanillaData);

        void resolveAndLaunch(const QString &instanceId, const VersionInfo &versionInfo);

        void doLaunch(const QString &instanceId, const VersionInfo &versionInfo);

        bool extractNatives(const QString &nativesDir, const QVector<LibraryInfo> &libs, QString *error);

        InstanceManager *instanceMgr_;
        CacheManager *cacheMgr_;
        JavaManager *javaMgr_;
        VersionManifest *manifest_;
        QMap<QString, QProcess *> runningProcesses_;
        QMap<QString, QStringList> recentOutputLines_;
        QMap<QString, QDateTime> processStartTimes_;
    };
}