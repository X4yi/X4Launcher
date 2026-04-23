#pragma once

#include "../core/Types.h"
#include <QString>
#include <QDateTime>
#include <QUuid>

namespace x4 {

struct Instance {
    QString id;             
    QString name;
    QString mcVersion;
    ModLoaderType loader = ModLoaderType::Vanilla;
    QString loaderVersion;
    InstanceState state = InstanceState::NeedsSetup;
    QDateTime created;
    QDateTime lastPlayed;
    QString iconKey;        
    QString group;          
    qint64 playTime = 0;    

    bool isValid() const { return !id.isEmpty() && !name.isEmpty(); }

    static Instance create(const QString& name, const QString& mcVersion,
                           ModLoaderType loader = ModLoaderType::Vanilla,
                           const QString& loaderVersion = {}) {
        Instance inst;
        inst.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        inst.name = name;
        inst.mcVersion = mcVersion;
        inst.loader = loader;
        inst.loaderVersion = loaderVersion;
        inst.state = InstanceState::NeedsSetup;
        inst.created = QDateTime::currentDateTime();
        return inst;
    }
};

} // namespace x4
