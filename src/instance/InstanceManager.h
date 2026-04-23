#pragma once

#include "Instance.h"
#include "InstanceConfig.h"
#include <QObject>
#include <QVector>
#include <QMap>

namespace x4 {


class InstanceManager : public QObject {
    Q_OBJECT
public:
    explicit InstanceManager(QObject* parent = nullptr);

    
    void loadAll();

    
    const QVector<Instance>& instances() const { return instances_; }

    
    const Instance* findById(const QString& id) const;
    Instance* findById(const QString& id);

    
    Result<Instance> createInstance(const QString& name, const QString& mcVersion,
                                    ModLoaderType loader = ModLoaderType::Vanilla,
                                    const QString& loaderVersion = QString(),
                                    const QString& iconKey = QStringLiteral("vanilla_icon"));

    
    Result<Instance> cloneInstance(const QString& sourceId, const QString& newName);

    
    Result<void> deleteInstance(const QString& id);

    
    Result<void> renameInstance(const QString& id, const QString& newName);

    
    QString instanceDir(const QString& id) const;

    
    QString minecraftDir(const QString& id) const;

    
    InstanceConfig loadConfig(const QString& id) const;

    
    bool saveConfig(const QString& id, const InstanceConfig& config);

    
    bool saveInstance(const Instance& inst);

    
    void setInstanceState(const QString& id, InstanceState state);

    
    void setInstanceGroup(const QString& id, const QString& group);

signals:
    void instanceAdded(const Instance& inst);
    void instanceRemoved(const QString& id);
    void instanceChanged(const Instance& inst);

private:
    Instance loadInstance(const QString& dir) const;
    bool saveInstanceMeta(const Instance& inst, const QString& dir) const;
    void ensureInstanceDirs(const QString& dir) const;
    static QString sanitizeName(const QString& name, const QString& id);

    QVector<Instance> instances_;
};

} 
