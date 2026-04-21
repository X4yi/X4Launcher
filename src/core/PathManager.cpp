#include "PathManager.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>

namespace x4 {

PathManager& PathManager::instance() {
    static PathManager pm;
    return pm;
}

void PathManager::init(const QString& portableDir) {
    baseDir_ = resolveBaseDir(portableDir, nullptr);
    dataDir_ = baseDir_;
    cacheDir_ = dataDir_ + QStringLiteral("/cache");

    // Keep relative file usage stable regardless of launcher invocation directory.
    if (!dataDir_.isEmpty()) {
        QDir::setCurrent(dataDir_);
    }

    
    ensureDir(dataDir_);
    ensureDir(cacheDir_);
    ensureDir(settingsDir());
    ensureDir(instancesDir());
    ensureDir(javaDir());
    ensureDir(librariesDir());
    ensureDir(assetsDir());
    ensureDir(versionsDir());
    ensureDir(nativesDir());
    ensureDir(logsDir());
}

bool PathManager::ensureDir(const QString& path) {
    QDir dir(path);
    if (!dir.exists()) {
        return dir.mkpath(QStringLiteral("."));
    }
    return true;
}

QString PathManager::resolveBaseDir(const QString& portableDir, bool* appImageMode) {
    if (appImageMode) {
        *appImageMode = false;
    }

    const QString explicitDir = portableDir.trimmed();
    if (!explicitDir.isEmpty()) {
        return QDir::cleanPath(QFileInfo(explicitDir).absoluteFilePath());
    }
    const QString appDir = QDir::cleanPath(QFileInfo(QCoreApplication::applicationDirPath()).absoluteFilePath());
    if (QFileInfo(appDir).fileName() == QStringLiteral("bin")) {
        return QDir::cleanPath(QFileInfo(appDir).absolutePath());
    }
    return appDir;
}

} 
