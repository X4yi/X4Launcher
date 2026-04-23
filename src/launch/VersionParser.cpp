#include "VersionParser.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QSysInfo>
#include <QOperatingSystemVersion>

namespace x4 {

Result<VersionInfo> VersionParser::parse(const QByteArray& json) {
    auto doc = QJsonDocument::fromJson(json);
    if (!doc.isObject())
        return Result<VersionInfo>::err(QStringLiteral("Invalid version JSON"));

    auto root = doc.object();
    VersionInfo info;

    info.id = root[QStringLiteral("id")].toString();
    info.mainClass = root[QStringLiteral("mainClass")].toString();
    info.type = root[QStringLiteral("type")].toString();
    info.inheritsFrom = root[QStringLiteral("inheritsFrom")].toString();
    auto assetIndex = root[QStringLiteral("assetIndex")].toObject();
    info.assetsVersion = assetIndex[QStringLiteral("id")].toString();
    info.assetIndexUrl = QUrl(assetIndex[QStringLiteral("url")].toString());
    info.assetIndexSha1 = assetIndex[QStringLiteral("sha1")].toString();

    if (info.assetsVersion.isEmpty())
        info.assetsVersion = root[QStringLiteral("assets")].toString();
    auto downloads = root[QStringLiteral("downloads")].toObject();
    auto client = downloads[QStringLiteral("client")].toObject();
    info.clientJarUrl = QUrl(client[QStringLiteral("url")].toString());
    info.clientJarSha1 = client[QStringLiteral("sha1")].toString();
    info.clientJarSize = client[QStringLiteral("size")].toInteger();
    info.libraries = parseLibraries(root[QStringLiteral("libraries")].toArray());

    
    auto args = root[QStringLiteral("arguments")].toObject();
    if (!args.isEmpty()) {
        info.gameArgTemplates = parseArguments(args[QStringLiteral("game")]);
        info.jvmArgTemplates = parseArguments(args[QStringLiteral("jvm")]);
    } else {
        
        QString legacyArgs = root[QStringLiteral("minecraftArguments")].toString();
        // Tokenize legacy minecraftArguments respecting quoted substrings
        auto tokenize = [](const QString& s) {
            QStringList out;
            QString cur;
            bool inQuote = false;
            QChar quoteChar;
            for (int i = 0; i < s.size(); ++i) {
                QChar c = s.at(i);
                if (!inQuote && (c == '"' || c == '\'')) {
                    inQuote = true;
                    quoteChar = c;
                    continue;
                }
                if (inQuote && c == quoteChar) {
                    inQuote = false;
                    continue;
                }
                if (!inQuote && c.isSpace()) {
                    if (!cur.isEmpty()) { out << cur; cur.clear(); }
                    continue;
                }
                cur.append(c);
            }
            if (!cur.isEmpty()) out << cur;
            return out;
        };
        info.gameArgTemplates = tokenize(legacyArgs);

        
        info.jvmArgTemplates = {
            QStringLiteral("-Djava.library.path=${natives_directory}"),
            QStringLiteral("-cp"),
            QStringLiteral("${classpath}")
        };
    }

    auto tweakers = root[QStringLiteral("+tweakers")].toArray();
    for (const auto& t : tweakers) {
        info.gameArgTemplates << QStringLiteral("--tweakClass") << t.toString();
    }

    if (info.mainClass.isEmpty() && info.inheritsFrom.isEmpty())
        return Result<VersionInfo>::err(QStringLiteral("Version JSON missing mainClass"));

    return Result<VersionInfo>::ok(std::move(info));
}

QVector<LibraryInfo> VersionParser::parseLibraries(const QJsonArray& arr) {
    QVector<LibraryInfo> libs;
    libs.reserve(arr.size());

    for (const auto& item : arr) {
        auto obj = item.toObject();

        if (obj.contains(QStringLiteral("rules"))) {
            if (!evaluateRules(obj[QStringLiteral("rules")].toArray()))
                continue;
        }

        LibraryInfo lib;
        lib.name = obj[QStringLiteral("name")].toString();

        
        auto downloads = obj[QStringLiteral("downloads")].toObject();
        auto artifact = downloads[QStringLiteral("artifact")].toObject();

        if (!artifact.isEmpty()) {
            lib.url = artifact[QStringLiteral("url")].toString();
            lib.sha1 = artifact[QStringLiteral("sha1")].toString();
            lib.size = artifact[QStringLiteral("size")].toInteger();
            lib.path = artifact[QStringLiteral("path")].toString();
        } else if (obj.contains(QStringLiteral("url"))) {
            QString baseUrl = obj[QStringLiteral("url")].toString();
            if (!baseUrl.endsWith('/')) baseUrl += '/';
            lib.url = baseUrl;
        }
        if (lib.path.isEmpty() && !lib.name.isEmpty()) {
            auto parts = lib.name.split(':');
            if (parts.size() >= 3) {
                QString group = parts[0]; group.replace('.', '/');
                lib.path = group + QStringLiteral("/") + parts[1] + QStringLiteral("/") +
                           parts[2] + QStringLiteral("/") + parts[1] + QStringLiteral("-") +
                           parts[2];
                if (parts.size() >= 4) {
                    lib.path += QStringLiteral("-") + parts[3];
                }
                lib.path += QStringLiteral(".jar");
            }
        }

        if (!lib.url.isEmpty() && !lib.path.isEmpty() && !lib.url.endsWith(".jar")) {
            lib.url += lib.path;
        }
        auto natives = obj[QStringLiteral("natives")].toObject();
        if (!natives.isEmpty()) {
#ifdef Q_OS_WIN
            QString classifier = natives[QStringLiteral("windows")].toString();
#elif defined(Q_OS_LINUX)
            QString classifier = natives[QStringLiteral("linux")].toString();
#elif defined(Q_OS_MACOS)
            QString classifier = natives[QStringLiteral("osx")].toString();
#else
            QString classifier;
#endif
            if (!classifier.isEmpty()) {
                classifier.replace(QStringLiteral("${arch}"), QSysInfo::currentCpuArchitecture());
                lib.isNative = true;
                lib.nativeClassifier = classifier;
                auto classifiers = downloads[QStringLiteral("classifiers")].toObject();
                auto nativeArtifact = classifiers[classifier].toObject();
                if (!nativeArtifact.isEmpty()) {
                    lib.url = nativeArtifact[QStringLiteral("url")].toString();
                    lib.sha1 = nativeArtifact[QStringLiteral("sha1")].toString();
                    lib.size = nativeArtifact[QStringLiteral("size")].toInteger();
                    lib.path = nativeArtifact[QStringLiteral("path")].toString();
                }
            }
        }

        if (!lib.name.isEmpty())
            libs.append(std::move(lib));
    }

    return libs;
}

QStringList VersionParser::parseArguments(const QJsonValue& val) {
    QStringList result;

    if (val.isArray()) {
        auto arr = val.toArray();
        for (const auto& item : arr) {
            if (item.isString()) {
                result << item.toString();
            } else if (item.isObject()) {
                auto obj = item.toObject();
                
                if (obj.contains(QStringLiteral("rules"))) {
                    if (!evaluateRules(obj[QStringLiteral("rules")].toArray()))
                        continue;
                }
                
                auto value = obj[QStringLiteral("value")];
                if (value.isString()) {
                    result << value.toString();
                } else if (value.isArray()) {
                    for (const auto& v : value.toArray()) {
                        result << v.toString();
                    }
                }
            }
        }
    }

    return result;
}

bool VersionParser::evaluateRules(const QJsonArray& rules) {
    bool allowed = false;

    for (const auto& item : rules) {
        auto rule = item.toObject();
        QString action = rule[QStringLiteral("action")].toString();
        bool matches = true;

        if (rule.contains(QStringLiteral("os"))) {
            matches = matchesCurrentOS(rule[QStringLiteral("os")].toObject());
        }

        if (rule.contains(QStringLiteral("features"))) {
            auto features = rule[QStringLiteral("features")].toObject();
            for (auto it = features.begin(); it != features.end(); ++it) {
                // By default we do not support/enable any optional launcher features 
                // like is_demo_user or has_custom_resolution, so they are false.
                bool featureEnabled = false;
                if (it.value().toBool() != featureEnabled) {
                    matches = false;
                    break;
                }
            }
        }

        if (action == QStringLiteral("allow")) {
            if (matches) allowed = true;
        } else if (action == QStringLiteral("disallow")) {
            if (matches) allowed = false;
        }
    }

    return allowed;
}

bool VersionParser::matchesCurrentOS(const QJsonObject& osObj) {
    if (osObj.contains(QStringLiteral("name"))) {
        QString osName = osObj[QStringLiteral("name")].toString();
#ifdef Q_OS_WIN
        if (osName != QStringLiteral("windows")) return false;
#elif defined(Q_OS_LINUX)
        if (osName != QStringLiteral("linux")) return false;
#elif defined(Q_OS_MACOS)
        if (osName != QStringLiteral("osx")) return false;
#endif
    }

    if (osObj.contains(QStringLiteral("arch"))) {
        QString arch = osObj[QStringLiteral("arch")].toString();
        QString current = QSysInfo::currentCpuArchitecture();
        if (arch == QStringLiteral("x86") && current != QStringLiteral("i386"))
            return false;
    }

    return true;
}

VersionInfo VersionParser::merge(const VersionInfo& parent, const VersionInfo& child) {
    VersionInfo merged = parent;

    if (!child.mainClass.isEmpty())
        merged.mainClass = child.mainClass;
    if (!child.id.isEmpty())
        merged.id = child.id;

    QVector<LibraryInfo> mergedLibs = child.libraries;
    mergedLibs.append(parent.libraries);
    merged.libraries = std::move(mergedLibs);

    if (!child.jvmArgTemplates.isEmpty()) {
        merged.jvmArgTemplates = child.jvmArgTemplates + parent.jvmArgTemplates;
    }
    if (!child.gameArgTemplates.isEmpty()) {
        merged.gameArgTemplates = parent.gameArgTemplates + child.gameArgTemplates;
    }

    return merged;
}

}