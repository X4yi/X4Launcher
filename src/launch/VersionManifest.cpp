#include "VersionManifest.h"
#include "../core/PathManager.h"
#include "../core/Logger.h"
#include "../network/HttpClient.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QFile>

namespace x4 {
    static const QString MANIFEST_URL =
            QStringLiteral("https://piston-meta.mojang.com/mc/game/version_manifest_v2.json");

    VersionManifest::VersionManifest(QObject *parent) : QObject(parent) {
    }

    void VersionManifest::fetch() {
        loadFromCache();

        auto *reply = HttpClient::instance().get(QUrl(MANIFEST_URL));
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            reply->deleteLater();

            if (reply->error() != QNetworkReply::NoError) {
                X4_WARN("Manifest", QStringLiteral("Fetch failed: %1").arg(reply->errorString()));
                if (!loaded_) {
                    emit fetchError(reply->errorString());
                }
                return;
            }

            QByteArray data = reply->readAll();
            saveToCache(data);

            auto doc = QJsonDocument::fromJson(data);
            auto root = doc.object();

            auto latest = root[QStringLiteral("latest")].toObject();
            latestRelease_ = latest[QStringLiteral("release")].toString();
            latestSnapshot_ = latest[QStringLiteral("snapshot")].toString();

            versions_.clear();
            auto arr = root[QStringLiteral("versions")].toArray();
            versions_.reserve(arr.size());

            for (const auto &item: arr) {
                auto obj = item.toObject();
                VersionEntry entry;
                entry.id = obj[QStringLiteral("id")].toString();
                entry.type = obj[QStringLiteral("type")].toString();
                entry.url = QUrl(obj[QStringLiteral("url")].toString());
                entry.sha1 = obj[QStringLiteral("sha1")].toString();
                entry.releaseTime = obj[QStringLiteral("releaseTime")].toString();
                versions_.append(std::move(entry));
            }

            loaded_ = true;
            X4_INFO("Manifest", QStringLiteral("Loaded %1 versions (latest: %2)")
                    .arg(versions_.size()).arg(latestRelease_));
            emit ready();
        });
    }

    const VersionEntry *VersionManifest::find(const QString &id) const {
        for (const auto &v: versions_) {
            if (v.id == id) return &v;
        }
        return nullptr;
    }

    void VersionManifest::loadFromCache() {
        QString cachePath = PathManager::instance().versionsDir() + QStringLiteral("/version_manifest_v2.json");
        QFile file(cachePath);
        if (!file.open(QIODevice::ReadOnly)) return;

        auto doc = QJsonDocument::fromJson(file.readAll());
        auto root = doc.object();

        auto latest = root[QStringLiteral("latest")].toObject();
        latestRelease_ = latest[QStringLiteral("release")].toString();
        latestSnapshot_ = latest[QStringLiteral("snapshot")].toString();

        auto arr = root[QStringLiteral("versions")].toArray();
        versions_.reserve(arr.size());
        for (const auto &item: arr) {
            auto obj = item.toObject();
            VersionEntry entry;
            entry.id = obj[QStringLiteral("id")].toString();
            entry.type = obj[QStringLiteral("type")].toString();
            entry.url = QUrl(obj[QStringLiteral("url")].toString());
            entry.sha1 = obj[QStringLiteral("sha1")].toString();
            entry.releaseTime = obj[QStringLiteral("releaseTime")].toString();
            versions_.append(std::move(entry));
        }

        if (!versions_.isEmpty()) {
            loaded_ = true;
            X4_DEBUG("Manifest", QStringLiteral("Loaded %1 versions from cache").arg(versions_.size()));
        }
    }

    void VersionManifest::saveToCache(const QByteArray &data) {
        QString cachePath = PathManager::instance().versionsDir() + QStringLiteral("/version_manifest_v2.json");
        QFile file(cachePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.write(data);
        }
    }
}