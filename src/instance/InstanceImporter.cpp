#include "InstanceImporter.h"
#include "InstanceManager.h"
#include "../core/PathManager.h"
#include "../core/Logger.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QTemporaryDir>

namespace x4 {

InstanceImporter::InstanceImporter(InstanceManager* instanceMgr, QObject* parent)
    : QObject(parent), instanceMgr_(instanceMgr) {}

Result<Instance> InstanceImporter::importZip(const QString& zipPath) {
    QFileInfo zipInfo(zipPath);
    if (!zipInfo.exists() || !zipInfo.isFile()) {
        return Result<Instance>::err(QStringLiteral("File not found: %1").arg(zipPath));
    }

    // Create a temporary directory for extraction
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        return Result<Instance>::err(QStringLiteral("Failed to create temporary directory"));
    }
    tempDir.setAutoRemove(true);

    emit importProgress(QStringLiteral("Extracting archive..."));

    // Extract the zip
    QString extractError;
    if (!extractZip(zipPath, tempDir.path(), extractError)) {
        return Result<Instance>::err(extractError);
    }
    QString extractedDir = tempDir.path();

    emit importProgress(QStringLiteral("Parsing instance metadata..."));

    // Parse metadata
    ParsedMeta meta = parseMeta(extractedDir);

    if (meta.mcVersion.isEmpty()) {
        return Result<Instance>::err(QStringLiteral(
            "Could not determine Minecraft version from the imported pack.\n"
            "Make sure this is a valid MultiMC / PrismLauncher export."));
    }

    if (meta.name.isEmpty()) {
        // Fallback to zip filename
        meta.name = zipInfo.completeBaseName();
    }

    emit importProgress(QStringLiteral("Creating instance '%1' (%2)...").arg(meta.name, meta.mcVersion));

    // Create the instance in X4Launcher
    auto createResult = instanceMgr_->createInstance(
        meta.name, meta.mcVersion, meta.loader, meta.loaderVersion,
        meta.iconKey.isEmpty() ? QStringLiteral("vanilla_icon") : meta.iconKey);

    if (createResult.isErr()) {
        return createResult;
    }

    auto& newInst = createResult.value();
    QString destMcDir = instanceMgr_->minecraftDir(newInst.id);

    emit importProgress(QStringLiteral("Copying game files..."));

    // Find the .minecraft or minecraft directory in the extracted content
    QStringList mcDirCandidates = {
        extractedDir + QStringLiteral("/.minecraft"),
        extractedDir + QStringLiteral("/minecraft"),
    };

    // Also check one level deeper (some zips have a root folder)
    QDir extractRoot(extractedDir);
    for (const auto& subDir : extractRoot.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        mcDirCandidates.append(extractedDir + QStringLiteral("/") + subDir + QStringLiteral("/.minecraft"));
        mcDirCandidates.append(extractedDir + QStringLiteral("/") + subDir + QStringLiteral("/minecraft"));
    }

    QString sourceMcDir;
    for (const auto& candidate : mcDirCandidates) {
        if (QDir(candidate).exists()) {
            sourceMcDir = candidate;
            break;
        }
    }

    if (!sourceMcDir.isEmpty()) {
        copyDirRecursive(sourceMcDir, destMcDir);
        X4_INFO("Import", QStringLiteral("Copied game files from %1").arg(sourceMcDir));
    } else {
        X4_WARN("Import", QStringLiteral("No .minecraft directory found in the archive. Instance created without game files."));
    }

    // Mark instance as ready (files already exist)
    newInst.state = InstanceState::Ready;
    instanceMgr_->saveInstance(newInst);

    emit importProgress(QStringLiteral("Import complete!"));
    X4_INFO("Import", QStringLiteral("Imported instance '%1' (%2) from %3")
        .arg(newInst.name, meta.mcVersion, zipInfo.fileName()));

    return createResult;
}

bool InstanceImporter::extractZip(const QString& zipPath, const QString& tempDir, QString& outError) {
    QProcess proc;
    proc.setWorkingDirectory(tempDir);

    // Try unzip first (most Linux systems have it)
    proc.start(QStringLiteral("unzip"), {QStringLiteral("-q"), QStringLiteral("-o"), zipPath, QStringLiteral("-d"), tempDir});
    if (!proc.waitForStarted(5000)) {
        // Fallback: try 7z
        proc.start(QStringLiteral("7z"), {QStringLiteral("x"), QStringLiteral("-y"), QStringLiteral("-o") + tempDir, zipPath});
        if (!proc.waitForStarted(5000)) {
            outError = QStringLiteral(
                "Could not find 'unzip' or '7z' command.\n"
                "Please install one of them (e.g., 'pacman -S unzip').");
            return false;
        }
    }

    if (!proc.waitForFinished(60000)) {
        proc.kill();
        outError = QStringLiteral("Extraction timed out");
        return false;
    }

    if (proc.exitCode() != 0) {
        outError = QStringLiteral("Extraction failed: %1")
            .arg(QString::fromUtf8(proc.readAllStandardError()));
        return false;
    }

    return true;
}

InstanceImporter::ParsedMeta InstanceImporter::parseMeta(const QString& extractedDir) {
    ParsedMeta meta;
    QDir dir(extractedDir);

    // Look for metadata files directly or one level deep
    QStringList searchDirs = {extractedDir};
    for (const auto& sub : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        searchDirs.append(extractedDir + QStringLiteral("/") + sub);
    }

    for (const auto& searchDir : searchDirs) {
        // Try instance.cfg first (MultiMC/PrismLauncher)
        QString cfgPath = searchDir + QStringLiteral("/instance.cfg");
        if (QFile::exists(cfgPath)) {
            meta = parseInstanceCfg(cfgPath);
        }

        // Try mmc-pack.json (overrides/supplements instance.cfg data)
        QString packPath = searchDir + QStringLiteral("/mmc-pack.json");
        if (QFile::exists(packPath)) {
            meta = parseMmcPack(packPath, meta);
        }

        // If we found a Minecraft version, we're done
        if (!meta.mcVersion.isEmpty()) break;
    }

    return meta;
}

InstanceImporter::ParsedMeta InstanceImporter::parseMmcPack(const QString& filePath, const ParsedMeta& base) {
    ParsedMeta meta = base;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return meta;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) return meta;

    QJsonObject root = doc.object();
    QJsonArray components = root.value(QStringLiteral("components")).toArray();

    for (const auto& comp : components) {
        QJsonObject obj = comp.toObject();
        QString uid = obj.value(QStringLiteral("uid")).toString();
        QString version = obj.value(QStringLiteral("version")).toString();

        if (uid == QStringLiteral("net.minecraft")) {
            meta.mcVersion = version;
        } else if (uid == QStringLiteral("net.fabricmc.fabric-loader")) {
            meta.loader = ModLoaderType::Fabric;
            meta.loaderVersion = version;
        } else if (uid == QStringLiteral("net.minecraftforge")) {
            meta.loader = ModLoaderType::Forge;
            meta.loaderVersion = version;
        } else if (uid == QStringLiteral("net.neoforged.neoforge") || uid == QStringLiteral("net.neoforged")) {
            meta.loader = ModLoaderType::NeoForge;
            meta.loaderVersion = version;
        } else if (uid == QStringLiteral("org.quiltmc.quilt-loader")) {
            meta.loader = ModLoaderType::Quilt;
            meta.loaderVersion = version;
        }
    }

    return meta;
}

InstanceImporter::ParsedMeta InstanceImporter::parseInstanceCfg(const QString& filePath) {
    ParsedMeta meta;

    // instance.cfg is an INI-like file
    QSettings cfg(filePath, QSettings::IniFormat);
    cfg.setFallbacksEnabled(false);

    // Some configs use [General] group, some don't
    meta.name = cfg.value(QStringLiteral("name")).toString();
    if (meta.name.isEmpty()) {
        cfg.beginGroup(QStringLiteral("General"));
        meta.name = cfg.value(QStringLiteral("name")).toString();
        cfg.endGroup();
    }

    meta.iconKey = cfg.value(QStringLiteral("iconKey")).toString();
    if (meta.iconKey.isEmpty()) {
        cfg.beginGroup(QStringLiteral("General"));
        meta.iconKey = cfg.value(QStringLiteral("iconKey")).toString();
        cfg.endGroup();
    }

    return meta;
}

bool InstanceImporter::copyDirRecursive(const QString& src, const QString& dst) {
    QDir srcDir(src);
    if (!srcDir.exists()) return false;

    QDir().mkpath(dst);

    // Copy files
    for (const auto& info : srcDir.entryInfoList(QDir::Files | QDir::Hidden)) {
        QString destFile = dst + QStringLiteral("/") + info.fileName();
        // Remove existing file first (QFile::copy won't overwrite)
        QFile::remove(destFile);
        QFile::copy(info.absoluteFilePath(), destFile);
    }

    // Recurse into subdirectories
    for (const auto& info : srcDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden)) {
        copyDirRecursive(info.absoluteFilePath(), dst + QStringLiteral("/") + info.fileName());
    }

    return true;
}

} // namespace x4
