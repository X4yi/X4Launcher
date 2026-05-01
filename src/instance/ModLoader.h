#pragma once

#include "../core/Types.h"
#include <QObject>
#include <QVector>

namespace x4 {
    struct ModLoaderVersion {
        QString version;
        QString mcVersion;
        bool stable = false;
    };


    class ModLoader : public QObject {
        Q_OBJECT

    public:
        explicit ModLoader(QObject *parent = nullptr) : QObject(parent) {
        }

        virtual ~ModLoader() = default;


        virtual void fetchVersions(const QString &mcVersion) = 0;


        virtual void install(const QString &instanceDir, const QString &mcVersion,
                             const QString &loaderVersion) = 0;

        virtual ModLoaderType type() const = 0;

        signals:
    

        void versionsReady(const QVector<ModLoaderVersion> &versions);

        void installFinished(bool success, const QString &error);
    };


    class FabricLoader : public ModLoader {
        Q_OBJECT

    public:
        explicit FabricLoader(QObject *parent = nullptr) : ModLoader(parent) {
        }

        void fetchVersions(const QString &mcVersion) override;

        void install(const QString &instanceDir, const QString &mcVersion,
                     const QString &loaderVersion) override;

        ModLoaderType type() const override { return ModLoaderType::Fabric; }
    };


    class VanillaLoader : public ModLoader {
        Q_OBJECT

    public:
        explicit VanillaLoader(QObject *parent = nullptr) : ModLoader(parent) {
        }

        void fetchVersions(const QString &) override { emit versionsReady({}); }

        void install(const QString &, const QString &, const QString &) override {
            emit installFinished(true, {});
        }

        ModLoaderType type() const override { return ModLoaderType::Vanilla; }
    };


    ModLoader *createModLoader(ModLoaderType type, QObject *parent = nullptr);
}