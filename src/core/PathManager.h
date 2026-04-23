#pragma once

#include <QString>
#include <QDir>

namespace x4 {



class PathManager {
public:
    static PathManager& instance();

    void init(const QString& portableDir = {});

    QString baseDir() const { return baseDir_; }
    
    
    QString dataDir() const { return dataDir_; }
    QString cacheDir() const { return cacheDir_; }
    QString settingsDir() const { return dataDir_ + QStringLiteral("/settings"); }
    QString launcherSettingsFile() const { return settingsDir() + QStringLiteral("/launcher.ini"); }
    QString authSettingsFile() const { return settingsDir() + QStringLiteral("/auth.ini"); }

    
    QString instancesDir() const { return dataDir_ + QStringLiteral("/instances"); }
    QString javaDir() const { return dataDir_ + QStringLiteral("/java"); }
    QString librariesDir() const { return cacheDir_ + QStringLiteral("/libraries"); }
    QString assetsDir() const { return cacheDir_ + QStringLiteral("/assets"); }
    QString versionsDir() const { return cacheDir_ + QStringLiteral("/versions"); }
    QString nativesDir() const { return cacheDir_ + QStringLiteral("/natives"); }
    QString logsDir() const { return dataDir_ + QStringLiteral("/logs"); }

    
    static bool ensureDir(const QString& path);

private:
    PathManager() = default;
    static QString resolveBaseDir(const QString& portableDir, bool* appImageMode);

    QString baseDir_;
    QString dataDir_;
    QString cacheDir_;
    
};

} 
