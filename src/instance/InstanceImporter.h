#pragma once

#include "Instance.h"
#include "../core/Types.h"
#include <QObject>

namespace x4 {

class InstanceManager;

/**
 * Imports instances from MultiMC / PrismLauncher .zip exports.
 *
 * Supports two common pack formats:
 *   - mmc-pack.json (PrismLauncher / MultiMC 5)
 *   - instance.cfg  (MultiMC legacy)
 *
 * The importer:
 *   1. Extracts the zip to a temporary directory
 *   2. Parses metadata for Minecraft version, loader, name
 *   3. Creates an X4Launcher instance
 *   4. Copies the .minecraft contents into the new instance
 */
class InstanceImporter : public QObject {
    Q_OBJECT
public:
    explicit InstanceImporter(InstanceManager* instanceMgr, QObject* parent = nullptr);

    /// Import a MultiMC / PrismLauncher .zip file
    Result<Instance> importZip(const QString& zipPath);

signals:
    void importProgress(const QString& status);

private:
    struct ParsedMeta {
        QString name;
        QString mcVersion;
        ModLoaderType loader = ModLoaderType::Vanilla;
        QString loaderVersion;
        QString iconKey;
    };

    /// Extract zip to tempDir, returns false on failure (sets outError)
    bool extractZip(const QString& zipPath, const QString& tempDir, QString& outError);

    /// Find and parse metadata from extracted files
    ParsedMeta parseMeta(const QString& extractedDir);

    /// Parse mmc-pack.json
    ParsedMeta parseMmcPack(const QString& filePath, const ParsedMeta& base);

    /// Parse instance.cfg
    ParsedMeta parseInstanceCfg(const QString& filePath);

    /// Recursively copy a directory
    bool copyDirRecursive(const QString& src, const QString& dst);

    InstanceManager* instanceMgr_;
};

} // namespace x4
