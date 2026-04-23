#include "ArgumentResolver.h"
#include "../auth/AccountManager.h"
#include "../core/SettingsManager.h"
#include <QDir>
#include <QUuid>
#include <QRegularExpression>

namespace x4 {

ArgumentResolver::LaunchArgs ArgumentResolver::resolve(
    const VersionInfo& version,
    const Instance& instance,
    const InstanceConfig& config,
    const QString& javaPath,
    const QStringList& classpathEntries,
    const QString& nativesDir,
    const QString& gameDir,
    const QString& assetsDir)
{
    LaunchArgs args;
    args.javaPath = javaPath;
    args.mainClass = version.mainClass;
    args.nativesDir = nativesDir;

#ifdef Q_OS_WIN
    args.classpath = classpathEntries.join(';');
#else
    args.classpath = classpathEntries.join(':');
#endif

    auto& settings = SettingsManager::instance();
    QString playerName = settings.playerName();
    if (playerName.isEmpty()) playerName = QStringLiteral("Player");

    QString offlineUuid = QUuid::createUuidV5(
        QUuid(QStringLiteral("OfflinePlayer")), playerName)
        .toString(QUuid::WithoutBraces);

    QString authPlayerName = playerName;
    QString authUuid = offlineUuid;
    QString authAccessToken = QStringLiteral("0");
    QString authXuid;
    QString authSession = QStringLiteral("token:0:0");
    QString userType = QStringLiteral("msa");

    const auto* account = auth::AccountManager::instance().account(config.accountId());
    if (account && account->isValid() && account->isPremium && !account->isExpired()) {
        authPlayerName = account->minecraftUsername;
        authUuid = account->uuid;
        authAccessToken = account->accessToken;
        authXuid = account->xuid;
        authSession = QStringLiteral("token:%1:%2").arg(authAccessToken, authUuid);
        userType = QStringLiteral("msa");
    }

    QMap<QString, QString> vars;
    vars[QStringLiteral("${auth_player_name}")] = authPlayerName;
    vars[QStringLiteral("${version_name}")] = instance.mcVersion;
    vars[QStringLiteral("${game_directory}")] = QDir::toNativeSeparators(gameDir);
    vars[QStringLiteral("${assets_root}")] = QDir::toNativeSeparators(assetsDir);
    vars[QStringLiteral("${assets_index_name}")] = version.assetsVersion;
    vars[QStringLiteral("${auth_uuid}")] = authUuid;
    vars[QStringLiteral("${auth_access_token}")] = authAccessToken;
    vars[QStringLiteral("${clientid}")] = QString();
    vars[QStringLiteral("${auth_xuid}")] = authXuid;
    vars[QStringLiteral("${user_type}")] = userType;
    vars[QStringLiteral("${version_type}")] = version.type;
    vars[QStringLiteral("${natives_directory}")] = QDir::toNativeSeparators(nativesDir);
    vars[QStringLiteral("${launcher_name}")] = QStringLiteral("X4Launcher");
    vars[QStringLiteral("${launcher_version}")] = QStringLiteral("0.1");
    vars[QStringLiteral("${classpath}")] = args.classpath;
    vars[QStringLiteral("${user_properties}")] = QStringLiteral("{}");
    vars[QStringLiteral("${game_assets}")] = QDir::toNativeSeparators(assetsDir);
    vars[QStringLiteral("${auth_session}")] = authSession;

    // Resolution placeholders (some legacy/new versions may expect these)
    int resW = config.effectiveWindowWidth();
    int resH = config.effectiveWindowHeight();
    if (resW <= 0) resW = 854;
    if (resH <= 0) resH = 480;
    vars[QStringLiteral("${resolution_width}")] = QString::number(resW);
    vars[QStringLiteral("${resolution_height}")] = QString::number(resH);

    int minMem = config.effectiveMinMemory();
    int maxMem = config.effectiveMaxMemory();

    args.jvmArgs << QStringLiteral("-XX:+IgnoreUnrecognizedVMOptions");
    args.jvmArgs << QStringLiteral("-Xms%1M").arg(minMem);
    args.jvmArgs << QStringLiteral("-Xmx%1M").arg(maxMem);

    if (instance.loader == ModLoaderType::Forge || instance.loader == ModLoaderType::NeoForge) {
        args.jvmArgs << QStringLiteral("-Dfml.ignoreInvalidMinecraftCertificates=true");
        args.jvmArgs << QStringLiteral("-Dfml.ignorePatchDiscrepancies=true");
    }

    QString customJvm = config.effectiveJvmArgs();
    if (!customJvm.isEmpty()) {
        // Tokenize custom JVM args respecting quotes
        QString cur; bool inQuote = false; QChar quoteChar;
        for (int i = 0; i < customJvm.size(); ++i) {
            QChar c = customJvm.at(i);
            if (!inQuote && (c == '"' || c == '\'')) { inQuote = true; quoteChar = c; continue; }
            if (inQuote && c == quoteChar) { inQuote = false; continue; }
            if (!inQuote && c.isSpace()) { if (!cur.isEmpty()) { args.jvmArgs << cur; cur.clear(); } continue; }
            cur.append(c);
        }
        if (!cur.isEmpty()) args.jvmArgs << cur;
    }

    args.jvmArgs << substituteAll(version.jvmArgTemplates, vars);

    args.gameArgs = substituteAll(version.gameArgTemplates, vars);

    QString customGameArgs = config.effectiveGameArgs();
    if (!customGameArgs.isEmpty()) {
        // Tokenize game args respecting quotes
        QString cur; bool inQuote = false; QChar quoteChar;
        QStringList toks;
        for (int i = 0; i < customGameArgs.size(); ++i) {
            QChar c = customGameArgs.at(i);
            if (!inQuote && (c == '"' || c == '\'')) { inQuote = true; quoteChar = c; continue; }
            if (inQuote && c == quoteChar) { inQuote = false; continue; }
            if (!inQuote && c.isSpace()) { if (!cur.isEmpty()) { toks << cur; cur.clear(); } continue; }
            cur.append(c);
        }
        if (!cur.isEmpty()) toks << cur;
        args.gameArgs << substituteAll(toks, vars);
    }
    if (config.effectiveFullscreen()) {
        args.gameArgs << QStringLiteral("--fullscreen");
    } else {
        int winW = config.effectiveWindowWidth();
        int winH = config.effectiveWindowHeight();
        if (winW > 0 && winH > 0) {
            args.gameArgs << QStringLiteral("--width") << QString::number(winW);
            args.gameArgs << QStringLiteral("--height") << QString::number(winH);
        }
    }

    return args;
}

QString ArgumentResolver::substitute(const QString& tmpl, const QMap<QString, QString>& vars) {
    QString result = tmpl;
    for (auto it = vars.constBegin(); it != vars.constEnd(); ++it) {
        result.replace(it.key(), it.value());
    }
    // Remove any remaining unresolved ${...} placeholders to avoid hard failures.
    result.remove(QRegularExpression(QStringLiteral(R"(\$\{[^}]+\})")));
    return result;
}

QStringList ArgumentResolver::substituteAll(const QStringList& templates,
                                             const QMap<QString, QString>& vars) {
    QStringList result;
    result.reserve(templates.size());
    for (const auto& t : templates) {
        result << substitute(t, vars);
    }
    return result;
}

}
