#pragma once

#include "LibraryCache.h"
#include "AssetCache.h"
#include <QObject>

namespace x4 {
    class CacheManager : public QObject {
        Q_OBJECT

    public:
        explicit CacheManager(QObject *parent = nullptr);

        LibraryCache &libraries() { return libCache_; }
        AssetCache &assets() { return assetCache_; }

    private:
        LibraryCache libCache_;
        AssetCache assetCache_;
    };
}