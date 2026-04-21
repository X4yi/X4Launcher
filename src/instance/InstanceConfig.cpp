#include "InstanceConfig.h"
#include "../core/SettingsManager.h"
#include <QFile>
#include <QJsonDocument>

namespace x4 {

InstanceConfig::InstanceConfig(const QString& instanceDir)
    : filePath_(instanceDir + QStringLiteral("/config.json"))
{
}

bool InstanceConfig::load() {
    QFile file(filePath_);
    if (!file.open(QIODevice::ReadOnly)) return false;
    auto doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) return false;
    data_ = doc.object();
    return true;
}

bool InstanceConfig::save() const {
    QFile file(filePath_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    file.write(QJsonDocument(data_).toJson(QJsonDocument::Indented));
    return true;
}

bool InstanceConfig::hasOverride(const QString& key) const {
    return data_.contains(key) && !data_[key].toVariant().isNull();
}

void InstanceConfig::clearOverride(const QString& key) {
    data_.remove(key);
}

int InstanceConfig::effectiveMinMemory() const {
    int v = minMemoryMB();
    return v > 0 ? v : SettingsManager::instance().minMemoryMB();
}

int InstanceConfig::effectiveMaxMemory() const {
    int v = maxMemoryMB();
    return v > 0 ? v : SettingsManager::instance().maxMemoryMB();
}

QString InstanceConfig::effectiveJavaPath() const {
    QString v = javaPath();
    return v.isEmpty() ? SettingsManager::instance().defaultJavaPath() : v;
}

QString InstanceConfig::effectiveJvmArgs() const {
    QString v = jvmArgs();
    return v.isEmpty() ? SettingsManager::instance().defaultJvmArgs() : v;
}

QString InstanceConfig::effectiveGameArgs() const {
    QString v = gameArgs();
    return v.isEmpty() ? SettingsManager::instance().defaultGameArgs() : v;
}

int InstanceConfig::effectiveWindowWidth() const {
    int v = windowWidth();
    return v > 0 ? v : SettingsManager::instance().gameWindowWidth();
}

int InstanceConfig::effectiveWindowHeight() const {
    int v = windowHeight();
    return v > 0 ? v : SettingsManager::instance().gameWindowHeight();
}

bool InstanceConfig::effectiveFullscreen() const {
    if (data_.contains(QStringLiteral("fullscreen"))) {
        return data_[QStringLiteral("fullscreen")].toBool();
    }
    return SettingsManager::instance().fullscreen();
}

} 
