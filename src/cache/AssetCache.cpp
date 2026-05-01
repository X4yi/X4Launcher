#include "AssetCache.h"
#include "../core/PathManager.h"
#include "../network/DownloadManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>

namespace x4 {
    static const QString ASSET_BASE_URL = QStringLiteral("https://resources.download.minecraft.net/");

    AssetCache::AssetCache(QObject *parent) : QObject(parent) {
    }

    QString AssetCache::objectPath(const QString &hash) const {
        return PathManager::instance().assetsDir() + QStringLiteral("/objects/") +
               hash.left(2) + QStringLiteral("/") + hash;
    }

    QString AssetCache::assetsRootDir() const {
        return PathManager::instance().assetsDir();
    }

    QVector<AssetInfo> AssetCache::parseAssetIndex(const QByteArray &json) {
        QVector<AssetInfo> result;
        auto doc = QJsonDocument::fromJson(json);
        auto objects = doc.object()[QStringLiteral("objects")].toObject();

        result.reserve(objects.size());
        for (auto it = objects.begin(); it != objects.end(); ++it) {
            auto obj = it.value().toObject();
            AssetInfo info;
            info.name = it.key();
            info.hash = obj[QStringLiteral("hash")].toString();
            info.size = obj[QStringLiteral("size")].toInteger();
            result.append(std::move(info));
        }
        return result;
    }

    bool AssetCache::has(const AssetInfo &asset) const {
        return QFileInfo::exists(objectPath(asset.hash));
    }

    QString AssetCache::getPath(const AssetInfo &asset) const {
        QString path = objectPath(asset.hash);
        if (QFileInfo::exists(path)) return path;
        return {};
    }

    void AssetCache::ensureCached(const QVector<AssetInfo> &assets, DownloadManager &dm) {
        for (const auto &asset: assets) {
            if (has(asset)) continue;

            QString url = ASSET_BASE_URL + asset.hash.left(2) + QStringLiteral("/") + asset.hash;
            dm.enqueue(QUrl(url), objectPath(asset.hash), asset.hash, asset.size);
        }
    }
}