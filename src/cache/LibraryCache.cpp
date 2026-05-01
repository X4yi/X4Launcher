#include "LibraryCache.h"
#include "../core/PathManager.h"
#include "../core/Logger.h"
#include "../network/DownloadManager.h"
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>

namespace x4 {
    LibraryCache::LibraryCache(QObject *parent) : QObject(parent) {
    }

    QString LibraryCache::storagePath(const LibraryInfo &lib) const {
        if (!lib.path.isEmpty()) {
            return PathManager::instance().librariesDir() + QStringLiteral("/") + lib.path;
        }
        auto parts = lib.name.split(':');
        if (parts.size() >= 3) {
            QString group = parts[0];
            group.replace('.', '/');
            QString filename = parts[1] + QStringLiteral("-") + parts[2];
            if (parts.size() >= 4)
                filename += QStringLiteral("-") + parts[3];
            filename += QStringLiteral(".jar");
            return PathManager::instance().librariesDir() + QStringLiteral("/") +
                   group + QStringLiteral("/") + parts[1] + QStringLiteral("/") +
                   parts[2] + QStringLiteral("/") + filename;
        }
        return {};
    }

    QString LibraryCache::getPath(const LibraryInfo &lib) const {
        QString path = storagePath(lib);
        if (QFileInfo::exists(path)) return path;
        return {};
    }

    bool LibraryCache::has(const LibraryInfo &lib) const {
        return !getPath(lib).isEmpty();
    }

    bool LibraryCache::ensureCached(const LibraryInfo &lib, DownloadManager &dm) {
        QString path = storagePath(lib);
        if (path.isEmpty()) return false;

        if (QFileInfo::exists(path)) {
            if (lib.sha1.isEmpty() || verify(lib)) {
                return true;
            }

            X4_WARN("Cache", QStringLiteral("Library hash mismatch, re-downloading: %1").arg(lib.name));
            QFile::remove(path);
        }


        if (!lib.url.isEmpty()) {
            dm.enqueue(QUrl(lib.url), path, lib.sha1, lib.size);
        }
        return false;
    }

    bool LibraryCache::verify(const LibraryInfo &lib) const {
        if (lib.sha1.isEmpty()) return true;

        QString path = storagePath(lib);
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return false;

        QCryptographicHash hash(QCryptographicHash::Sha1);
        hash.addData(&file);
        return hash.result().toHex() == lib.sha1.toLatin1();
    }
}