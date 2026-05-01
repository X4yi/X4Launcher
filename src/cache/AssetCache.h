#pragma once

#include "../core/Types.h"
#include <QObject>
#include <QVector>

namespace x4 {
    class DownloadManager;


    class AssetCache : public QObject {
        Q_OBJECT

    public:
        explicit AssetCache(QObject *parent = nullptr);


        static QVector<AssetInfo> parseAssetIndex(const QByteArray &json);


        bool has(const AssetInfo &asset) const;


        QString getPath(const AssetInfo &asset) const;


        void ensureCached(const QVector<AssetInfo> &assets, DownloadManager &dm);


        QString assetsRootDir() const;

    private:
        QString objectPath(const QString &hash) const;
    };
}