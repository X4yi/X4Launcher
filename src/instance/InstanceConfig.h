#pragma once

#include <QJsonObject>
#include <QString>
#include <QVariant>

namespace x4 {



class InstanceConfig {
public:
    InstanceConfig() = default;
    explicit InstanceConfig(const QString& instanceDir);

    
    bool load();
    
    bool save() const;

    
    QString javaPath() const { return data_[QStringLiteral("javaPath")].toString(); }
    void setJavaPath(const QString& p) { data_[QStringLiteral("javaPath")] = p; }

    int minMemoryMB() const { return data_[QStringLiteral("minMemory")].toInt(); }
    void setMinMemoryMB(int mb) { data_[QStringLiteral("minMemory")] = mb; }

    int maxMemoryMB() const { return data_[QStringLiteral("maxMemory")].toInt(); }
    void setMaxMemoryMB(int mb) { data_[QStringLiteral("maxMemory")] = mb; }

    QString jvmArgs() const { return data_[QStringLiteral("jvmArgs")].toString(); }
    void setJvmArgs(const QString& args) { data_[QStringLiteral("jvmArgs")] = args; }

    QString gameArgs() const { return data_[QStringLiteral("gameArgs")].toString(); }
    void setGameArgs(const QString& args) { data_[QStringLiteral("gameArgs")] = args; }

    QString accountId() const { return data_[QStringLiteral("accountId")].toString(); }
    void setAccountId(const QString& id) { data_[QStringLiteral("accountId")] = id; }

    int windowWidth() const { return data_[QStringLiteral("windowWidth")].toInt(); }
    void setWindowWidth(int w) { data_[QStringLiteral("windowWidth")] = w; }

    int windowHeight() const { return data_[QStringLiteral("windowHeight")].toInt(); }
    void setWindowHeight(int h) { data_[QStringLiteral("windowHeight")] = h; }

    bool fullscreen() const { return data_.contains(QStringLiteral("fullscreen")) ? data_[QStringLiteral("fullscreen")].toBool() : false; }
    void setFullscreen(bool f) { data_[QStringLiteral("fullscreen")] = f; }

    
    int effectiveMinMemory() const;
    int effectiveMaxMemory() const;
    QString effectiveJavaPath() const;
    QString effectiveJvmArgs() const;
    QString effectiveGameArgs() const;
    int effectiveWindowWidth() const;
    int effectiveWindowHeight() const;
    bool effectiveFullscreen() const;

    bool hasOverride(const QString& key) const;
    void clearOverride(const QString& key);

private:
    QString filePath_;
    QJsonObject data_;
};

} 
