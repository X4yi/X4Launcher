#pragma once

#include "../core/Types.h"
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>

namespace x4 {
    struct ModrinthProject {
        QString id;
        QString slug;
        QString title;
        QString description;
        QString iconUrl;
        QString author;
        int downloads = 0;
        QString projectType;
        QStringList categories;
    };

    struct ModrinthFile {
        QString url;
        QString filename;
        QString sha1;
        qint64 size = 0;
        bool primary = false;
    };

    struct ModrinthDependency {
        QString versionId;
        QString projectId;
        QString dependencyType;
    };

    struct ModrinthVersion {
        QString id;
        QString projectId;
        QString name;
        QString versionNumber;
        QStringList gameVersions;
        QStringList loaders;
        QVector<ModrinthFile> files;
        QVector<ModrinthDependency> dependencies;

        QString primaryUrl() const {
            for (const auto &file: files) {
                if (file.primary) return file.url;
            }
            if (!files.isEmpty()) return files.first().url;
            return QString();
        }
    };

    struct ModrinthSearchResult {
        QVector<ModrinthProject> hits;
        int totalHits = 0;
        int offset = 0;
        int limit = 0;
    };

    class ModrinthAPI : public QObject {
        Q_OBJECT

    public:
        explicit ModrinthAPI(QObject *parent = nullptr);

        ~ModrinthAPI();

        static QString buildFacets(const QString &mcVersion, ModLoaderType loader, const QString &projectType);

        void searchProjects(const QString &query, const QString &facets, int limit = 20, int offset = 0,
                            const QString &index = "relevance");

        void getProjectVersions(const QString &projectId, const QString &mcVersion = QString(),
                                const QString &loader = QString());

        void getVersion(const QString &versionId);

        void batchGetByHashes(const QStringList &sha1List);

        void batchGetProjects(const QStringList &projectIds);

        void resolveDependencies(const QStringList &versionIds, const QString &mcVersion, ModLoaderType loader);

        signals:
    

        void searchFinished(const x4::ModrinthSearchResult &result);

        void searchFailed(const QString &error);

        void versionsFinished(const QString &projectId, const QVector<x4::ModrinthVersion> &versions);

        void versionsFailed(const QString &projectId, const QString &error);

        void versionFinished(const x4::ModrinthVersion &version);

        void versionFailed(const QString &versionId, const QString &error);

        void batchHashesFinished(const QMap<QString, x4::ModrinthVersion> &hashVersionMap);

        void batchHashesFailed(const QString &error);

        void batchProjectsFinished(const QMap<QString, x4::ModrinthProject> &projectMap);

        void batchProjectsFailed(const QString &error);

        void dependenciesResolved(const QVector<x4::ModrinthVersion> &requiredVersions);

        void dependenciesResolveFailed(const QString &error);

    private:
        void setupRequest(QNetworkRequest &req);

        ModrinthProject parseProject(const QJsonObject &obj);

        ModrinthVersion parseVersion(const QJsonObject &obj);

        QNetworkAccessManager *networkManager_;
        const QString API_URL = "https://api.modrinth.com/v2";
        const QString USER_AGENT = "X4Launcher/0.1";
    };
}