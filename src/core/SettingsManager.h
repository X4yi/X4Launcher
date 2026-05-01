#pragma once

#include <QSettings>
#include <QObject>
#include <QMutex>
#include <QHash>

namespace x4 {
    class SettingsManager : public QObject {
        Q_OBJECT

    public:
        static SettingsManager &instance();

        // Basic typed accessors
        QString defaultJavaPath() const;

        void setDefaultJavaPath(const QString &path);

        bool autoDetectJava() const;

        void setAutoDetectJava(bool enabled);

        int minMemoryMB() const;

        void setMinMemoryMB(int mb);

        int maxMemoryMB() const;

        void setMaxMemoryMB(int mb);

        QString defaultJvmArgs() const;

        void setDefaultJvmArgs(const QString &args);

        QString defaultGameArgs() const;

        void setDefaultGameArgs(const QString &args);

        QString theme() const;

        void setTheme(const QString &theme);

        bool zenMode() const;

        void setZenMode(bool zen);

        QString language() const;

        void setLanguage(const QString &lang);

        QString playerName() const;

        void setPlayerName(const QString &name);

        int downloadParallelism() const;

        void setDownloadParallelism(int n);

        int gameWindowWidth() const;

        void setGameWindowWidth(int w);

        int gameWindowHeight() const;

        void setGameWindowHeight(int h);

        bool fullscreen() const;

        void setFullscreen(bool f);

        int closeOnLaunch() const;

        void setCloseOnLaunch(int val);

        bool showSnapshots() const;

        void setShowSnapshots(bool show);

        bool showOldAlpha() const;

        void setShowOldAlpha(bool show);

        int logLevel() const;

        void setLogLevel(int level);

        // Persist pending changes to disk (manual save)
        void saveNow();

        signals:
    

        void settingChanged(const QString &key);

    private:
        SettingsManager();

        QVariant get(const QString &key, const QVariant &defaultValue) const;

        void set(const QString &key, const QVariant &value);

        QSettings settings_;
        mutable QMutex mutex_;
        QHash<QString, QVariant> pending_; // values waiting to be flushed by saveNow()
    };
}