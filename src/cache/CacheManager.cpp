#include "CacheManager.h"

namespace x4 {
    CacheManager::CacheManager(QObject *parent)
        : QObject(parent)
          , libCache_(this)
          , assetCache_(this) {
    }
}