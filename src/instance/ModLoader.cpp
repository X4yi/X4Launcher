#include "ModLoader.h"
#include "../network/HttpClient.h"
#include "../core/Logger.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QFile>
#include <QDir>

namespace x4 {
    void FabricLoader::fetchVersions(const QString &mcVersion) {
        QString url = QStringLiteral("https://meta.fabricmc.net/v2/versions/loader/%1").arg(mcVersion);
        auto *reply = HttpClient::instance().get(QUrl(url));

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            reply->deleteLater();
            QVector<ModLoaderVersion> versions;

            if (reply->error() != QNetworkReply::NoError) {
                X4_WARN("Fabric", QStringLiteral("Failed to fetch versions: %1").arg(reply->errorString()));
                emit versionsReady(versions);
                return;
            }

            auto doc = QJsonDocument::fromJson(reply->readAll());
            auto arr = doc.array();

            for (const auto &item: arr) {
                auto obj = item.toObject();
                auto loader = obj[QStringLiteral("loader")].toObject();

                ModLoaderVersion ver;
                ver.version = loader[QStringLiteral("version")].toString();
                ver.stable = loader[QStringLiteral("stable")].toBool();
                versions.append(ver);
            }

            emit versionsReady(versions);
        });
    }

    void FabricLoader::install(const QString &instanceDir, const QString &mcVersion,
                               const QString &loaderVersion) {
        QString url = QStringLiteral(
                    "https://meta.fabricmc.net/v2/versions/loader/%1/%2/profile/json")
                .arg(mcVersion, loaderVersion);

        auto *reply = HttpClient::instance().get(QUrl(url));
        connect(reply, &QNetworkReply::finished, this, [this, reply, instanceDir, mcVersion, loaderVersion]() {
            reply->deleteLater();

            if (reply->error() != QNetworkReply::NoError) {
                emit installFinished(
                    false, QStringLiteral("Failed to download Fabric profile: %1").arg(reply->errorString()));
                return;
            }

            QByteArray data = reply->readAll();


            QString versionId = QStringLiteral("fabric-loader-%1-%2").arg(loaderVersion, mcVersion);
            QString versionDir = instanceDir + QStringLiteral("/versions/") + versionId;
            QDir().mkpath(versionDir);

            QFile file(versionDir + QStringLiteral("/") + versionId + QStringLiteral(".json"));
            if (!file.open(QIODevice::WriteOnly)) {
                emit installFinished(false, QStringLiteral("Failed to write Fabric version JSON"));
                return;
            }
            file.write(data);
            file.close();

            X4_INFO("Fabric", QStringLiteral("Installed Fabric %1 for MC %2").arg(loaderVersion, mcVersion));
            emit installFinished(true, {});
        });
    }


    ModLoader *createModLoader(ModLoaderType type, QObject *parent) {
        switch (type) {
            case ModLoaderType::Fabric:
                return new FabricLoader(parent);
            case ModLoaderType::Vanilla:
            default:
                return new VanillaLoader(parent);
        }
    }
}