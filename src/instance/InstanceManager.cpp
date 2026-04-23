#include "InstanceManager.h"
#include "../core/PathManager.h"
#include "../core/Logger.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace x4 {

InstanceManager::InstanceManager(QObject* parent) : QObject(parent) {}

void InstanceManager::loadAll() {
    instances_.clear();
    QDir dir(PathManager::instance().instancesDir());
    auto entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const auto& entry : entries) {
        QString instDir = dir.absoluteFilePath(entry);
        auto inst = loadInstance(instDir);
        if (inst.isValid()) {
            instances_.append(inst);
        }
    }

    X4_INFO("Instance", QStringLiteral("Loaded %1 instance(s)").arg(instances_.size()));
}

Instance InstanceManager::loadInstance(const QString& dir) const {
    QFile file(dir + QStringLiteral("/instance.json"));
    if (!file.open(QIODevice::ReadOnly)) return {};

    auto doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) return {};

    auto obj = doc.object();
    Instance inst;
    inst.id = obj[QStringLiteral("id")].toString();
    inst.name = obj[QStringLiteral("name")].toString();
    inst.mcVersion = obj[QStringLiteral("mcVersion")].toString();
    inst.loader = static_cast<ModLoaderType>(obj[QStringLiteral("loader")].toInt());
    inst.loaderVersion = obj[QStringLiteral("loaderVersion")].toString();
    inst.state = static_cast<InstanceState>(obj[QStringLiteral("state")].toInt());
    inst.created = QDateTime::fromString(obj[QStringLiteral("created")].toString(), Qt::ISODate);
    inst.lastPlayed = QDateTime::fromString(obj[QStringLiteral("lastPlayed")].toString(), Qt::ISODate);
    inst.iconKey = obj[QStringLiteral("iconKey")].toString();
    inst.group = obj[QStringLiteral("group")].toString();
    inst.playTime = obj[QStringLiteral("playTime")].toVariant().toLongLong();

    
    if (inst.state == InstanceState::Running || inst.state == InstanceState::Downloading) {
        inst.state = InstanceState::Ready;
    }

    return inst;
}

bool InstanceManager::saveInstanceMeta(const Instance& inst, const QString& dir) const {
    QJsonObject obj;
    obj[QStringLiteral("id")] = inst.id;
    obj[QStringLiteral("name")] = inst.name;
    obj[QStringLiteral("mcVersion")] = inst.mcVersion;
    obj[QStringLiteral("loader")] = static_cast<int>(inst.loader);
    obj[QStringLiteral("loaderVersion")] = inst.loaderVersion;
    obj[QStringLiteral("state")] = static_cast<int>(inst.state);
    obj[QStringLiteral("created")] = inst.created.toString(Qt::ISODate);
    obj[QStringLiteral("lastPlayed")] = inst.lastPlayed.toString(Qt::ISODate);
    obj[QStringLiteral("iconKey")] = inst.iconKey;
    obj[QStringLiteral("group")] = inst.group;
    obj[QStringLiteral("playTime")] = inst.playTime;

    QFile file(dir + QStringLiteral("/instance.json"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    return true;
}

void InstanceManager::ensureInstanceDirs(const QString& dir) const {
    QDir d(dir);
    d.mkpath(QStringLiteral("minecraft/mods"));
    d.mkpath(QStringLiteral("minecraft/saves"));
    d.mkpath(QStringLiteral("minecraft/resourcepacks"));
    d.mkpath(QStringLiteral("minecraft/config"));
    d.mkpath(QStringLiteral("minecraft/screenshots"));
    d.mkpath(QStringLiteral("logs"));
}

QString InstanceManager::sanitizeName(const QString& name, const QString& id) {
    QString safe = name;
    safe.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]")), QStringLiteral("_"));
    safe = safe.trimmed();
    if (safe.isEmpty())
        safe = QStringLiteral("instance_%1").arg(id.left(8));
    return safe;
}

QString InstanceManager::instanceDir(const QString& id) const {
    QDir base(PathManager::instance().instancesDir());
    for (const auto& entry : base.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString candidate = base.absoluteFilePath(entry);
        QFileInfo metaInfo(candidate + QStringLiteral("/instance.json"));
        if (!metaInfo.exists()) continue;
        QFile meta(metaInfo.absoluteFilePath());
        if (!meta.open(QIODevice::ReadOnly)) continue;
        auto doc = QJsonDocument::fromJson(meta.readAll());
        if (doc.object()[QStringLiteral("id")].toString() == id)
            return candidate;
    }
    if (const Instance* inst = findById(id))
        return base.absolutePath() + QStringLiteral("/") + sanitizeName(inst->name, id);
    return base.absolutePath() + QStringLiteral("/") + id;
}

QString InstanceManager::minecraftDir(const QString& id) const {
    return instanceDir(id) + QStringLiteral("/minecraft");
}

const Instance* InstanceManager::findById(const QString& id) const {
    for (const auto& inst : instances_) {
        if (inst.id == id) return &inst;
    }
    return nullptr;
}

Instance* InstanceManager::findById(const QString& id) {
    for (auto& inst : instances_) {
        if (inst.id == id) return &inst;
    }
    return nullptr;
}

Result<Instance> InstanceManager::createInstance(const QString& name, const QString& mcVersion,
                                                  ModLoaderType loader, const QString& loaderVersion, const QString& iconKey) {
    if (name.trimmed().isEmpty())
        return Result<Instance>::err(QStringLiteral("Instance name cannot be empty"));

    auto inst = Instance::create(name.trimmed(), mcVersion, loader, loaderVersion);
    inst.iconKey = iconKey;
    QString dir = PathManager::instance().instancesDir() + QStringLiteral("/") + sanitizeName(inst.name, inst.id);

    if (!PathManager::ensureDir(dir)) {
        return Result<Instance>::err(QStringLiteral("Failed to create instance directory"));
    }

    ensureInstanceDirs(dir);

    if (!saveInstanceMeta(inst, dir)) {
        return Result<Instance>::err(QStringLiteral("Failed to save instance metadata"));
    }

    
    InstanceConfig config(dir);
    config.save();

    instances_.append(inst);
    emit instanceAdded(inst);

    X4_INFO("Instance", QStringLiteral("Created instance: %1 (%2)").arg(inst.name, inst.mcVersion));
    return Result<Instance>::ok(inst);
}

Result<Instance> InstanceManager::cloneInstance(const QString& sourceId, const QString& newName) {
    const auto* source = findById(sourceId);
    if (!source)
        return Result<Instance>::err(QStringLiteral("Source instance not found"));

    auto result = createInstance(newName, source->mcVersion, source->loader, source->loaderVersion);
    if (result.isErr()) return result;

    auto& newInst = result.value();
    QString srcMC = minecraftDir(sourceId);
    QString dstMC = minecraftDir(newInst.id);

    
    
    auto copyDir = [](const QString& src, const QString& dst) {
        QDir srcDir(src);
        if (!srcDir.exists()) return;
        QDir().mkpath(dst);
        for (const auto& entry : srcDir.entryInfoList(QDir::Files)) {
            QFile::copy(entry.absoluteFilePath(), dst + QStringLiteral("/") + entry.fileName());
        }
    };

    copyDir(srcMC + QStringLiteral("/mods"), dstMC + QStringLiteral("/mods"));
    copyDir(srcMC + QStringLiteral("/config"), dstMC + QStringLiteral("/config"));
    copyDir(srcMC + QStringLiteral("/resourcepacks"), dstMC + QStringLiteral("/resourcepacks"));

    
    QString srcConfig = instanceDir(sourceId) + QStringLiteral("/config.json");
    QString dstConfig = instanceDir(newInst.id) + QStringLiteral("/config.json");
    QFile::copy(srcConfig, dstConfig);

    return result;
}

Result<void> InstanceManager::deleteInstance(const QString& id) {
    auto* inst = findById(id);
    if (!inst)
        return Result<void>::err(QStringLiteral("Instance not found"));

    if (inst->state == InstanceState::Running)
        return Result<void>::err(QStringLiteral("Cannot delete a running instance"));

    QString dir = instanceDir(id);
    QDir d(dir);
    if (!d.removeRecursively()) {
        return Result<void>::err(QStringLiteral("Failed to delete instance directory"));
    }

    instances_.erase(std::remove_if(instances_.begin(), instances_.end(),
        [&id](const Instance& i) { return i.id == id; }), instances_.end());

    emit instanceRemoved(id);
    X4_INFO("Instance", QStringLiteral("Deleted instance: %1").arg(id));
    return Result<void>::ok();
}

Result<void> InstanceManager::renameInstance(const QString& id, const QString& newName) {
    auto* inst = findById(id);
    if (!inst)
        return Result<void>::err(QStringLiteral("Instance not found"));

    inst->name = newName.trimmed();
    saveInstance(*inst);
    emit instanceChanged(*inst);
    return Result<void>::ok();
}

bool InstanceManager::saveInstance(const Instance& inst) {
    return saveInstanceMeta(inst, instanceDir(inst.id));
}

void InstanceManager::setInstanceState(const QString& id, InstanceState state) {
    auto* inst = findById(id);
    if (!inst) return;
    inst->state = state;
    saveInstance(*inst);
    emit instanceChanged(*inst);
}

void InstanceManager::setInstanceGroup(const QString& id, const QString& group) {
    auto* inst = findById(id);
    if (!inst) return;
    inst->group = group;
    saveInstance(*inst);
    emit instanceChanged(*inst);
}

InstanceConfig InstanceManager::loadConfig(const QString& id) const {
    InstanceConfig config(instanceDir(id));
    config.load();
    return config;
}

bool InstanceManager::saveConfig(const QString& id, const InstanceConfig& config) {
    return config.save();
}

} 
