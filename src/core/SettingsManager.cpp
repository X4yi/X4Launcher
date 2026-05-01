#include "SettingsManager.h"
#include "PathManager.h"

namespace x4 {
    SettingsManager &SettingsManager::instance() {
        static SettingsManager sm;
        return sm;
    }

    SettingsManager::SettingsManager()
        : settings_([] {
            auto &pm = PathManager::instance();
            if (pm.dataDir().isEmpty()) {
                pm.init();
            }
            return pm.launcherSettingsFile();
        }(), QSettings::IniFormat) {
        settings_.setFallbacksEnabled(false);
    }

    QVariant SettingsManager::get(const QString &key, const QVariant &defaultValue) const {
        QMutexLocker lock(&mutex_);
        if (pending_.contains(key)) return pending_.value(key);
        return settings_.value(key, defaultValue);
    }

    void SettingsManager::set(const QString &key, const QVariant &value) {
        {
            QMutexLocker lock(&mutex_);
            pending_.insert(key, value);
        }
        emit settingChanged(key);
    }


    QString SettingsManager::defaultJavaPath() const {
        return get(QStringLiteral("java/path"), QString()).toString();
    }

    void SettingsManager::setDefaultJavaPath(const QString &path) {
        set(QStringLiteral("java/path"), path);
    }

    bool SettingsManager::autoDetectJava() const {
        return get(QStringLiteral("java/autoDetect"), true).toBool();
    }

    void SettingsManager::setAutoDetectJava(bool enabled) {
        set(QStringLiteral("java/autoDetect"), enabled);
    }


    int SettingsManager::minMemoryMB() const {
        return get(QStringLiteral("minecraft/minMemory"), 512).toInt();
    }

    void SettingsManager::setMinMemoryMB(int mb) {
        set(QStringLiteral("minecraft/minMemory"), mb);
    }

    int SettingsManager::maxMemoryMB() const {
        return get(QStringLiteral("minecraft/maxMemory"), 2048).toInt();
    }

    void SettingsManager::setMaxMemoryMB(int mb) {
        set(QStringLiteral("minecraft/maxMemory"), mb);
    }


    QString SettingsManager::defaultJvmArgs() const {
        return get(QStringLiteral("minecraft/jvmArgs"), QString()).toString();
    }

    void SettingsManager::setDefaultJvmArgs(const QString &args) {
        set(QStringLiteral("minecraft/jvmArgs"), args);
    }

    QString SettingsManager::defaultGameArgs() const {
        return get(QStringLiteral("minecraft/gameArgs"), QString()).toString();
    }

    void SettingsManager::setDefaultGameArgs(const QString &args) {
        set(QStringLiteral("minecraft/gameArgs"), args);
    }


    QString SettingsManager::theme() const {
        return get(QStringLiteral("ui/theme"), QStringLiteral("dark")).toString();
    }

    void SettingsManager::setTheme(const QString &theme) {
        set(QStringLiteral("ui/theme"), theme);
    }

    bool SettingsManager::zenMode() const {
        return get(QStringLiteral("ui/zenMode"), false).toBool();
    }

    void SettingsManager::setZenMode(bool zen) {
        set(QStringLiteral("ui/zenMode"), zen);
    }

    QString SettingsManager::language() const {
        return get(QStringLiteral("ui/language"), QStringLiteral("en")).toString();
    }

    void SettingsManager::setLanguage(const QString &lang) {
        set(QStringLiteral("ui/language"), lang);
    }


    QString SettingsManager::playerName() const {
        return get(QStringLiteral("player/name"), QStringLiteral("Player")).toString();
    }

    void SettingsManager::setPlayerName(const QString &name) {
        set(QStringLiteral("player/name"), name);
    }


    int SettingsManager::downloadParallelism() const {
        return get(QStringLiteral("network/parallelism"), 6).toInt();
    }

    void SettingsManager::setDownloadParallelism(int n) {
        set(QStringLiteral("network/parallelism"), qBound(1, n, 20));
    }


    int SettingsManager::gameWindowWidth() const {
        return get(QStringLiteral("minecraft/windowWidth"), 854).toInt();
    }

    void SettingsManager::setGameWindowWidth(int w) {
        set(QStringLiteral("minecraft/windowWidth"), w);
    }

    int SettingsManager::gameWindowHeight() const {
        return get(QStringLiteral("minecraft/windowHeight"), 480).toInt();
    }

    void SettingsManager::setGameWindowHeight(int h) {
        set(QStringLiteral("minecraft/windowHeight"), h);
    }

    bool SettingsManager::fullscreen() const {
        return get(QStringLiteral("minecraft/fullscreen"), false).toBool();
    }

    void SettingsManager::setFullscreen(bool f) {
        set(QStringLiteral("minecraft/fullscreen"), f);
    }

    int SettingsManager::closeOnLaunch() const {
        return get(QStringLiteral("ui/closeOnLaunch"), 0).toInt();
    }

    void SettingsManager::setCloseOnLaunch(int val) {
        set(QStringLiteral("ui/closeOnLaunch"), val);
    }


    bool SettingsManager::showSnapshots() const {
        return get(QStringLiteral("versions/showSnapshots"), false).toBool();
    }

    void SettingsManager::setShowSnapshots(bool show) {
        set(QStringLiteral("versions/showSnapshots"), show);
    }

    bool SettingsManager::showOldAlpha() const {
        return get(QStringLiteral("versions/showOldAlpha"), false).toBool();
    }

    void SettingsManager::setShowOldAlpha(bool show) {
        set(QStringLiteral("versions/showOldAlpha"), show);
    }

    int SettingsManager::logLevel() const {
        return get(QStringLiteral("system/logLevel"), 0).toInt();
    }

    void SettingsManager::setLogLevel(int level) {
        set(QStringLiteral("system/logLevel"), level);
    }

    void SettingsManager::saveNow() {
        QMutexLocker lock(&mutex_);
        for (auto it = pending_.begin(); it != pending_.end(); ++it) {
            settings_.setValue(it.key(), it.value());
        }
        pending_.clear();
        settings_.sync();
    }
}