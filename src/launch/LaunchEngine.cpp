#include "LaunchEngine.h"
#include "../instance/InstanceManager.h"
#include "../core/PathManager.h"
#include "../core/Logger.h"
#include "../network/HttpClient.h"
#include <QNetworkReply>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QRegularExpression>
#include <QThread>
#include <algorithm>
#include <cstring>
#include "MetaAPI.h"
#include "../core/GlobalEvents.h"

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <QStandardPaths>
#include <QProcessEnvironment>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace x4 {
    namespace {
        bool isSafeChildPath(const QString &basePath, const QString &childPath) {
            const QString baseCanon = QFileInfo(basePath).canonicalFilePath();
            const QString childCanon = QFileInfo(childPath).canonicalFilePath();
            if (baseCanon.isEmpty()) return false;
            if (!childCanon.isEmpty()) {
                if (childCanon == baseCanon) return true;
                return childCanon.startsWith(baseCanon + QLatin1Char('/'));
            }
            // child does not exist yet (e.g., planned destination). Fall back to absolute paths.
            const QString baseAbs = QDir(basePath).absolutePath();
            const QString childAbs = QDir(childPath).absolutePath();
            if (childAbs == baseAbs) return true;
            return childAbs.startsWith(baseAbs + QLatin1Char('/'));
        }

        bool safeRemoveRecursively(const QString &basePath, const QString &targetPath) {
            if (!isSafeChildPath(basePath, targetPath)) {
                X4_ERROR("Launch",
                         QStringLiteral("Refusing to remove path outside of base: %1 (base=%2)").arg(
                             targetPath, basePath));
                return false;
            }
            return QDir(targetPath).removeRecursively();
        }

#ifdef Q_OS_LINUX
        bool isWaylandSession(const QProcessEnvironment &env) {
            return env.value(QStringLiteral("XDG_SESSION_TYPE"), QString()).compare(
                       QStringLiteral("wayland"), Qt::CaseInsensitive) == 0
                   || !env.value(QStringLiteral("WAYLAND_DISPLAY")).trimmed().isEmpty();
        }

        QString buildDisplayError() {
            return QStringLiteral(
                "X11 DISPLAY no disponible o no accesible (instala/activa xorg-xwayland o usa una sesión X11).");
        }

        QString detectKnownCrashReason(const QStringList &lines) {
            for (const QString &line: lines) {
                if (line.contains(QStringLiteral("Could not open X display connection"), Qt::CaseInsensitive)) {
                    return buildDisplayError();
                }
                if (line.contains(QStringLiteral("Can't connect to X11 window server"), Qt::CaseInsensitive)) {
                    return buildDisplayError();
                }
                if (line.contains(QStringLiteral("No X11 DISPLAY variable was set"), Qt::CaseInsensitive)) {
                    return buildDisplayError();
                }
                if (line.contains(QStringLiteral("OutOfMemoryError"), Qt::CaseInsensitive)) {
                    return QStringLiteral("Java agotó memoria. Ajusta -Xmx/-Xms o reduce mods/shaders.");
                }
                if (line.contains(QStringLiteral("Unable to launch"), Qt::CaseInsensitive)) {
                    return QStringLiteral(
                        "LaunchWrapper no pudo iniciar Minecraft. Revisa stacktrace y dependencias/mods.");
                }
            }
            return {};
        }

        QString displayNumberFromDisplay(const QString &display) {
            if (display.isEmpty() || !display.startsWith(QStringLiteral(":"))) {
                return {};
            }
            int end = 1;
            while (end < display.size() && display.at(end).isDigit()) {
                ++end;
            }
            if (end <= 1) {
                return {};
            }
            return display.mid(1, end - 1);
        }

        bool displaySocketExists(const QString &display) {
            const QString number = displayNumberFromDisplay(display);
            if (number.isEmpty()) {
                return false;
            }
            return QFileInfo::exists(QStringLiteral("/tmp/.X11-unix/X") + number);
        }

        bool canReachXServerSocket(const QString &display) {
            const QString number = displayNumberFromDisplay(display);
            if (number.isEmpty()) {
                return false;
            }
            const QByteArray path = (QStringLiteral("/tmp/.X11-unix/X") + number).toUtf8();
            if (path.size() >= static_cast<int>(sizeof(sockaddr_un::sun_path))) {
                return false;
            }

            int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
            if (fd < 0) {
                return false;
            }

            sockaddr_un addr{};
            addr.sun_family = AF_UNIX;
            std::memcpy(addr.sun_path, path.constData(), static_cast<size_t>(path.size()));
            addr.sun_path[path.size()] = '\0';

            const int rc = ::connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
            ::close(fd);
            return rc == 0;
        }

        QStringList xauthorityCandidates(const QProcessEnvironment &env) {
            QStringList candidates;
            const QString current = env.value(QStringLiteral("XAUTHORITY")).trimmed();
            if (!current.isEmpty() && QFileInfo::exists(current)) {
                candidates << current;
            }

            QStringList runtimeDirs;
            const QString xdgRuntimeDir = env.value(QStringLiteral("XDG_RUNTIME_DIR")).trimmed();
            if (!xdgRuntimeDir.isEmpty()) {
                runtimeDirs << xdgRuntimeDir;
            }
            const QString homeRuntime = QStringLiteral("/run/user/%1").arg(static_cast<qulonglong>(::getuid()));
            if (!homeRuntime.endsWith('/') && !homeRuntime.endsWith('\\') && QFileInfo::exists(homeRuntime)) {
                runtimeDirs << homeRuntime;
            }

            for (const QString &runtimeDir: runtimeDirs) {
                QDir dir(runtimeDir);
                if (!dir.exists()) {
                    continue;
                }
                const QFileInfoList files = dir.entryInfoList(
                    QStringList() << QStringLiteral(".mutter-Xwaylandauth.*")
                    << QStringLiteral(".Xauthority*")
                    << QStringLiteral("xauth_*"),
                    QDir::Files,
                    QDir::Time);
                for (const QFileInfo &fi: files) {
                    candidates << fi.absoluteFilePath();
                }
            }

            const QString homeAuth = QDir::homePath() + QStringLiteral("/.Xauthority");
            if (QFileInfo::exists(homeAuth)) {
                candidates << homeAuth;
            }

            candidates.removeDuplicates();
            return candidates;
        }

        QStringList displaysFromX11Sockets() {
            const QDir x11Dir(QStringLiteral("/tmp/.X11-unix"));
            if (!x11Dir.exists()) {
                return {};
            }

            QStringList out;
            const QFileInfoList entries = x11Dir.entryInfoList(
                QStringList() << QStringLiteral("X*"),
                QDir::System | QDir::Files | QDir::NoDotAndDotDot);
            for (const QFileInfo &fi: entries) {
                const QString name = fi.fileName();
                if (!name.startsWith(QStringLiteral("X"))) {
                    continue;
                }
                bool ok = false;
                const int displayNum = name.mid(1).toInt(&ok);
                if (ok && displayNum >= 0) {
                    out << QStringLiteral(":%1").arg(displayNum);
                }
            }
            out.removeDuplicates();
            return out;
        }

        QStringList displaysFromXAuth(const QString &xauthPath) {
            if (xauthPath.isEmpty()) {
                return {};
            }

            const QString xauthExe = QStandardPaths::findExecutable(QStringLiteral("xauth"));
            if (xauthExe.isEmpty()) {
                return {};
            }

            QProcess proc;
            proc.start(xauthExe, {QStringLiteral("-f"), xauthPath, QStringLiteral("list")});
            if (!proc.waitForFinished(1500) || proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
                return {};
            }

            QStringList out;
            const QStringList lines = QString::fromUtf8(proc.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
            for (const QString &line: lines) {
                const int colon = line.lastIndexOf(':');
                if (colon < 0) {
                    continue;
                }
                int start = colon + 1;
                int end = start;
                while (end < line.size() && line.at(end).isDigit()) {
                    ++end;
                }
                if (end <= start) {
                    continue;
                }
                bool ok = false;
                const int displayNum = line.mid(start, end - start).toInt(&ok);
                if (ok && displayNum >= 0) {
                    out << QStringLiteral(":%1").arg(displayNum);
                }
            }

            out.removeDuplicates();
            return out;
        }

        QString x11ProbeExecutable() {
            QString probe = QStandardPaths::findExecutable(QStringLiteral("xdpyinfo"));
            if (!probe.isEmpty()) {
                return probe;
            }
            probe = QStandardPaths::findExecutable(QStringLiteral("xset"));
            return probe;
        }

        bool canConnectToDisplay(const QProcessEnvironment &baseEnv,
                                 const QString &display,
                                 const QString &xauthPath) {
            if (display.isEmpty()) {
                return false;
            }
            if (!displaySocketExists(display)) {
                return false;
            }

            const QString probeExe = x11ProbeExecutable();
            QStringList probeArgs;
            if (probeExe.endsWith(QStringLiteral("xdpyinfo"))) {
                probeArgs << QStringLiteral("-display") << display;
            } else {
                probeArgs << QStringLiteral("-display") << display << QStringLiteral("q");
            }

            if (probeExe.isEmpty()) {
                return canReachXServerSocket(display);
            }

            QProcess probe;
            auto probeEnv = baseEnv;
            probeEnv.insert(QStringLiteral("DISPLAY"), display);
            if (!xauthPath.isEmpty()) {
                probeEnv.insert(QStringLiteral("XAUTHORITY"), xauthPath);
            } else {
                probeEnv.remove(QStringLiteral("XAUTHORITY"));
            }
            probe.setProcessEnvironment(probeEnv);
            probe.start(probeExe, probeArgs);
            if (!probe.waitForFinished(2000)) {
                probe.kill();
                return false;
            }
            return probe.exitStatus() == QProcess::NormalExit && probe.exitCode() == 0;
        }

        QString firstFreeDisplay() {
            for (int i = 0; i < 100; ++i) {
                const QString d = QStringLiteral(":%1").arg(i);
                if (!displaySocketExists(d)) {
                    return d;
                }
            }
            return {};
        }

        bool tryStartXWaylandServer(const QProcessEnvironment &env,
                                    QString *displayOut,
                                    QString *reasonOut) {
            const QString xwaylandExe = QStandardPaths::findExecutable(QStringLiteral("Xwayland"));
            if (xwaylandExe.isEmpty()) {
                if (reasonOut) {
                    *reasonOut = QStringLiteral("Xwayland executable not found.");
                }
                return false;
            }

            const QString display = firstFreeDisplay();
            if (display.isEmpty()) {
                if (reasonOut) {
                    *reasonOut = QStringLiteral("No free X display number found.");
                }
                return false;
            }

            const QStringList args = {
                display,
                QStringLiteral("-rootless"),
                QStringLiteral("-terminate"),
                QStringLiteral("-nolisten"), QStringLiteral("tcp"),
                QStringLiteral("-ac")
            };

            qint64 pid = 0;
            if (!QProcess::startDetached(xwaylandExe, args, QString(), &pid)) {
                if (reasonOut) {
                    *reasonOut = QStringLiteral("Failed to start Xwayland process.");
                }
                return false;
            }

            const QString probeExe = x11ProbeExecutable();
            for (int i = 0; i < 60; ++i) {
                if (displaySocketExists(display)) {
                    if (!probeExe.isEmpty()) {
                        if (canConnectToDisplay(env, display, QString())) {
                            if (displayOut) {
                                *displayOut = display;
                            }
                            return true;
                        }
                    } else {
                        if (displayOut) {
                            *displayOut = display;
                        }
                        return true;
                    }
                }
                QThread::msleep(100);
            }

            if (reasonOut) {
                *reasonOut = QStringLiteral("Xwayland started but display did not become ready.");
            }
            return false;
        }

        struct X11Binding {
            QString display;
            QString xauthority;
        };

        QString readProcFileText(const QString &path) {
            QFile f(path);
            if (!f.open(QIODevice::ReadOnly)) {
                return {};
            }
            return QString::fromUtf8(f.readAll());
        }

        QString envValueFromBlob(const QByteArray &envBlob, const QByteArray &key) {
            const QList<QByteArray> parts = envBlob.split('\0');
            const QByteArray prefix = key + '=';
            for (const QByteArray &p: parts) {
                if (p.startsWith(prefix)) {
                    return QString::fromUtf8(p.mid(prefix.size()));
                }
            }
            return {};
        }

        QString extractDisplayFromCmdline(const QString &cmdline) {
            const QStringList parts = cmdline.split('\0', Qt::SkipEmptyParts);
            for (const QString &p: parts) {
                if (p.startsWith(QStringLiteral(":")) && !displayNumberFromDisplay(p).isEmpty()) {
                    return p;
                }
            }
            return {};
        }

        QVector<X11Binding> xwaylandBindingsFromProc() {
            QVector<X11Binding> out;
            QDir procDir(QStringLiteral("/proc"));
            const QStringList pids = procDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString &pid: pids) {
                bool ok = false;
                pid.toLongLong(&ok);
                if (!ok) {
                    continue;
                }

                const QString cmdline = readProcFileText(QStringLiteral("/proc/%1/cmdline").arg(pid));
                if (cmdline.isEmpty() || !cmdline.contains(QStringLiteral("Xwayland"), Qt::CaseInsensitive)) {
                    continue;
                }

                QFile envFile(QStringLiteral("/proc/%1/environ").arg(pid));
                QString display = extractDisplayFromCmdline(cmdline);
                QString xauth;
                if (envFile.open(QIODevice::ReadOnly)) {
                    const QByteArray blob = envFile.readAll();
                    if (display.isEmpty()) {
                        display = envValueFromBlob(blob, "DISPLAY");
                    }
                    xauth = envValueFromBlob(blob, "XAUTHORITY");
                }
                if (display.isEmpty()) {
                    continue;
                }

                X11Binding b;
                b.display = display;
                b.xauthority = xauth;
                out.append(b);
            }

            std::sort(out.begin(), out.end(), [](const X11Binding &a, const X11Binding &b) {
                return displayNumberFromDisplay(a.display).toInt() < displayNumberFromDisplay(b.display).toInt();
            });
            return out;
        }

        X11Binding resolveUsableX11Binding(const QProcessEnvironment &env, const QString &currentDisplay) {
            const QVector<X11Binding> procBindings = xwaylandBindingsFromProc();
            for (const X11Binding &b: procBindings) {
                if (!b.xauthority.isEmpty() && canConnectToDisplay(env, b.display, b.xauthority)) {
                    return b;
                }
                if (canConnectToDisplay(env, b.display, QString())) {
                    return {b.display, QString()};
                }
            }

            const QStringList xauthCandidates = xauthorityCandidates(env);
            const QString currentXauth = env.value(QStringLiteral("XAUTHORITY")).trimmed();

            QStringList displayCandidates;
            if (!currentDisplay.isEmpty()) {
                displayCandidates << currentDisplay;
            }
            for (const QString &xauth: xauthCandidates) {
                for (const QString &d: displaysFromXAuth(xauth)) {
                    if (!displayCandidates.contains(d)) {
                        displayCandidates << d;
                    }
                }
            }
            for (const QString &d: displaysFromX11Sockets()) {
                if (!displayCandidates.contains(d)) {
                    displayCandidates << d;
                }
            }

            for (const QString &d: displayCandidates) {
                if (!currentXauth.isEmpty() && canConnectToDisplay(env, d, currentXauth)) {
                    return {d, currentXauth};
                }
                if (canConnectToDisplay(env, d, QString())) {
                    return {d, QString()};
                }
                for (const QString &xauth: xauthCandidates) {
                    if (canConnectToDisplay(env, d, xauth)) {
                        return {d, xauth};
                    }
                }
            }

            if (!currentDisplay.isEmpty() && canReachXServerSocket(currentDisplay)) {
                return {currentDisplay, currentXauth};
            }
            return {};
        }
#endif // Q_OS_LINUX

        QString extractCrashReportFromOutput(const QStringList &lines) {
            for (auto it = lines.crbegin(); it != lines.crend(); ++it) {
                const QString line = *it;
                const int idx = line.indexOf(QStringLiteral("/crash-reports/"));
                if (idx < 0) {
                    continue;
                }

                int start = idx;
                while (start > 0 && !line.at(start - 1).isSpace()) {
                    --start;
                }

                int end = line.indexOf(QStringLiteral(".txt"), idx);
                if (end < 0) {
                    continue;
                }
                QString path = line.mid(start, end - start + 4).trimmed();
                path.replace(QStringLiteral("#@!@#"), QString());
                return path.trimmed();
            }
            return {};
        }

        QString findRecentCrashReport(const QString &gameDir, const QDateTime &processStartAt) {
            const QString crashDirPath = gameDir + QStringLiteral("/crash-reports");
            QDir crashDir(crashDirPath);
            if (!crashDir.exists()) {
                return {};
            }

            const QFileInfoList files = crashDir.entryInfoList(
                QStringList() << QStringLiteral("*.txt"),
                QDir::Files,
                QDir::Time);
            if (files.isEmpty()) {
                return {};
            }

            const QDateTime threshold = processStartAt.addSecs(-10);
            for (const QFileInfo &fi: files) {
                if (!fi.lastModified().isValid() || fi.lastModified() >= threshold) {
                    return fi.absoluteFilePath();
                }
            }
            return {};
        }

        QString buildCrashSummaryMessage(int exitCode, const QString &knownReason, const QString &crashReportPath) {
            QString message = QStringLiteral("GAME_CRASH: Minecraft crashed (exit code %1)").arg(exitCode);
            if (!knownReason.isEmpty()) {
                message += QStringLiteral(" | Cause: %1").arg(knownReason);
            }
            if (!crashReportPath.isEmpty()) {
                message += QStringLiteral(" | Crash report: %1").arg(crashReportPath);
            }
            return message;
        }

        QString extractUnresolvedPlaceholder(const QStringList &args) {
            for (const QString &arg: args) {
                const int idx = arg.indexOf(QStringLiteral("${"));
                if (idx >= 0) {
                    return arg.mid(idx);
                }
            }
            return {};
        }

        int parseMemoryMb(const QString &arg, const QString &key) {
            if (!arg.startsWith(key)) {
                return -1;
            }
            const QString raw = arg.mid(key.size()).trimmed();
            if (raw.isEmpty()) {
                return -1;
            }

            QRegularExpression re(QStringLiteral(R"(^(\d+)([kKmMgG]?)$)"));
            auto m = re.match(raw);
            if (!m.hasMatch()) {
                return -1;
            }

            const qint64 value = m.captured(1).toLongLong();
            const QString unit = m.captured(2).toLower();
            if (unit == QStringLiteral("g")) {
                return static_cast<int>(value * 1024);
            }
            if (unit == QStringLiteral("k")) {
                return static_cast<int>(value / 1024);
            }
            return static_cast<int>(value);
        }

        QString validateLaunchInputs(const VersionInfo &versionInfo,
                                     const JavaVersion &java,
                                     const QString &mcVersion,
                                     const QString &gameDir,
                                     const QString &assetsDir,
                                     const QString &nativesDir,
                                     const QStringList &classpath,
                                     const QStringList &jvmArgs,
                                     const QStringList &gameArgs) {
            if (mcVersion.trimmed().isEmpty()) {
                return QStringLiteral("GAME_LAUNCH_ERROR: Minecraft version is empty.");
            }
            if (!java.isValid() || !QFileInfo(java.path).exists() || !QFileInfo(java.path).isExecutable()) {
                return QStringLiteral("GAME_LAUNCH_ERROR: Java path is invalid or not executable.");
            }
            if (versionInfo.mainClass.trimmed().isEmpty()) {
                return QStringLiteral("GAME_LAUNCH_ERROR: mainClass is empty in resolved version metadata.");
            }

            if (!QDir(gameDir).exists() && !QDir().mkpath(gameDir)) {
                return QStringLiteral("GAME_LAUNCH_ERROR: Game directory is missing and cannot be created.");
            }
            if (!QDir(assetsDir).exists()) {
                return QStringLiteral("GAME_LAUNCH_ERROR: Assets directory does not exist.");
            }
            if (!QDir(nativesDir).exists()) {
                return QStringLiteral("GAME_LAUNCH_ERROR: Natives directory does not exist.");
            }

            if (classpath.isEmpty()) {
                return QStringLiteral("GAME_LAUNCH_ERROR: Resolved classpath is empty.");
            }
            for (const QString &cp: classpath) {
                QFileInfo fi(cp);
                if (!fi.exists() || !fi.isFile() || fi.size() <= 0) {
                    return QStringLiteral("GAME_LAUNCH_ERROR: Missing classpath entry: %1").arg(cp);
                }
            }

            const QString unresolvedJvm = extractUnresolvedPlaceholder(jvmArgs);
            if (!unresolvedJvm.isEmpty()) {
                return QStringLiteral("GAME_LAUNCH_ERROR: Unresolved JVM placeholder in args: %1").arg(unresolvedJvm);
            }
            const QString unresolvedGame = extractUnresolvedPlaceholder(gameArgs);
            if (!unresolvedGame.isEmpty()) {
                return QStringLiteral("GAME_LAUNCH_ERROR: Unresolved game placeholder in args: %1").arg(unresolvedGame);
            }

            int xms = -1;
            int xmx = -1;
            for (const QString &arg: jvmArgs) {
                const int parsedXms = parseMemoryMb(arg, QStringLiteral("-Xms"));
                if (parsedXms >= 0) {
                    xms = parsedXms;
                }
                const int parsedXmx = parseMemoryMb(arg, QStringLiteral("-Xmx"));
                if (parsedXmx >= 0) {
                    xmx = parsedXmx;
                }
            }
            if (xms > 0 && xmx > 0 && xmx < xms) {
                return QStringLiteral("GAME_LAUNCH_ERROR: Invalid memory args (Xmx < Xms).");
            }

            return {};
        }
    } // namespace

    LaunchEngine::LaunchEngine(InstanceManager *instanceMgr, CacheManager *cacheMgr,
                               JavaManager *javaMgr, VersionManifest *manifest,
                               QObject *parent)
        : QObject(parent)
          , instanceMgr_(instanceMgr)
          , cacheMgr_(cacheMgr)
          , javaMgr_(javaMgr)
          , manifest_(manifest) {
    }

    bool LaunchEngine::isRunning(const QString &instanceId) const {
        return runningProcesses_.contains(instanceId);
    }

    void LaunchEngine::kill(const QString &instanceId) {
        auto *proc = runningProcesses_.value(instanceId);
        if (proc) {
            proc->kill();
        }
    }

    void LaunchEngine::launch(const QString &instanceId) {
        auto *inst = instanceMgr_->findById(instanceId);
        if (!inst) {
            emit launchFailed(instanceId, QStringLiteral("Instance not found"));
            return;
        }

        if (isRunning(instanceId)) {
            emit launchFailed(instanceId, QStringLiteral("Instance is already running"));
            return;
        }

        emit launchProgress(instanceId, QStringLiteral("Resolving version..."));
        GlobalEvents::instance().stageProgress(QStringLiteral("Resolving version..."));

        QString versionJsonPath = PathManager::instance().versionsDir() +
                                  QStringLiteral("/%1/%1.json").arg(inst->mcVersion);

        if (QFile::exists(versionJsonPath)) {
            QFile file(versionJsonPath);
            if (file.open(QIODevice::ReadOnly)) {
                onVersionJsonReady(instanceId, file.readAll());
                return;
            }
        }

        auto *entry = manifest_->find(inst->mcVersion);
        if (!entry) {
            emit launchFailed(instanceId, QStringLiteral("Version %1 not found in manifest").arg(inst->mcVersion));
            return;
        }

        auto *reply = HttpClient::instance().get(entry->url);
        connect(reply, &QNetworkReply::finished, this,
                [this, reply, instanceId, versionJsonPath]() {
                    reply->deleteLater();
                    if (reply->error() != QNetworkReply::NoError) {
                        emit launchFailed(instanceId, QStringLiteral("Failed to download version JSON: %1")
                                          .arg(reply->errorString()));
                        return;
                    }

                    QByteArray data = reply->readAll();

                    QDir().mkpath(QFileInfo(versionJsonPath).absolutePath());
                    QFile file(versionJsonPath);
                    if (file.open(QIODevice::WriteOnly)) {
                        file.write(data);
                        file.close();
                    }

                    onVersionJsonReady(instanceId, data);
                });
    }

    void LaunchEngine::onVersionJsonReady(const QString &instanceId, const QByteArray &data) {
        auto *inst = instanceMgr_->findById(instanceId);
        if (!inst) {
            emit launchFailed(instanceId, QStringLiteral("Instance disappeared"));
            return;
        }

        if (inst->loader != ModLoaderType::Vanilla) {
            fetchLoaderMeta(instanceId, data);
            return;
        }

        auto result = VersionParser::parse(data);
        if (result.isErr()) {
            emit launchFailed(instanceId, QStringLiteral("Failed to parse version JSON: %1").arg(result.error()));
            return;
        }

        resolveAndLaunch(instanceId, result.value());
    }

    void LaunchEngine::fetchLoaderMeta(const QString &instanceId, const QByteArray &vanillaData) {
        auto *inst = instanceMgr_->findById(instanceId);
        if (!inst) return;

        QString uid;
        switch (inst->loader) {
            case ModLoaderType::Fabric: uid = "net.fabricmc.fabric-loader";
                break;
            case ModLoaderType::Forge: uid = "net.minecraftforge";
                break;
            case ModLoaderType::NeoForge: uid = "net.neoforged";
                break;
            case ModLoaderType::Quilt: uid = "org.quiltmc.quilt-loader";
                break;
            default: break;
        }

        if (uid.isEmpty()) {
            emit launchFailed(instanceId, QStringLiteral("Unknown Mod Loader Type"));
            return;
        }

        emit launchProgress(instanceId, QStringLiteral("Fetching Mod Loader metadata..."));
        GlobalEvents::instance().stageProgress(QStringLiteral("Fetching Mod Loader metadata..."));
        auto *meta = new MetaAPI(this);

        auto handlePatchJson = [this, meta, instanceId, vanillaData, inst](const QByteArray &patchData) {
            meta->deleteLater();
            auto result = VersionParser::parse(vanillaData);
            if (result.isErr()) {
                emit launchFailed(instanceId, QStringLiteral("Failed to parse vanilla JSON"));
                return;
            }

            auto patchResult = VersionParser::parse(patchData);
            if (patchResult.isErr()) {
                emit launchFailed(
                    instanceId, QStringLiteral("Failed to parse mod loader JSON: %1").arg(patchResult.error()));
                return;
            }

            auto merged = VersionParser::merge(result.value(), patchResult.value());
            resolveAndLaunch(instanceId, merged);
        };

        if (inst->loaderVersion.isEmpty()) {
            connect(meta, &MetaAPI::indexFinished, this,
                    [this, meta, instanceId, inst, uid, handlePatchJson](const QString &, const QByteArray &idxData) {
                        auto doc = QJsonDocument::fromJson(idxData);
                        auto versions = doc.object()["versions"].toArray();
                        QString bestVersion;

                        for (auto item: versions) {
                            auto vobj = item.toObject();
                            auto reqs = vobj["requires"].toArray();
                            for (auto reqItem: reqs) {
                                auto r = reqItem.toObject();
                                if (r["uid"].toString() == "net.minecraft" && r["equals"].toString() == inst->
                                    mcVersion) {
                                    if (bestVersion.isEmpty() || vobj["recommended"].toBool()) {
                                        bestVersion = vobj["version"].toString();
                                    }
                                }
                            }
                        }

                        if (bestVersion.isEmpty()) {
                            emit launchFailed(
                                instanceId,
                                QStringLiteral("No compatible %1 version found for Minecraft %2").arg(
                                    uid, inst->mcVersion));
                            meta->deleteLater();
                            return;
                        }

                        inst->loaderVersion = bestVersion;
                        instanceMgr_->saveInstance(*inst);

                        connect(meta, &MetaAPI::versionFinished, this,
                                [this, handlePatchJson](const QString &, const QString &, const QByteArray &pdata) {
                                    handlePatchJson(pdata);
                                });
                        connect(meta, &MetaAPI::versionFailed, this,
                                [this, meta, instanceId](const QString &, const QString &, const QString &err) {
                                    emit launchFailed(instanceId, "Mod Loader DL Error: " + err);
                                    meta->deleteLater();
                                });
                        meta->getPackageVersion(uid, bestVersion);
                    });

            connect(meta, &MetaAPI::indexFailed, this, [this, meta, instanceId](const QString &, const QString &err) {
                emit launchFailed(instanceId, "Mod Loader Index DL Error: " + err);
                meta->deleteLater();
            });

            meta->getPackageIndex(uid);
        } else {
            connect(meta, &MetaAPI::versionFinished, this,
                    [this, handlePatchJson](const QString &, const QString &, const QByteArray &pdata) {
                        handlePatchJson(pdata);
                    });
            connect(meta, &MetaAPI::versionFailed, this,
                    [this, meta, instanceId](const QString &, const QString &, const QString &err) {
                        emit launchFailed(instanceId, "Mod Loader DL Error: " + err);
                        meta->deleteLater();
                    });
            meta->getPackageVersion(uid, inst->loaderVersion);
        }
    }

    void LaunchEngine::resolveAndLaunch(const QString &instanceId, const VersionInfo &versionInfo) {
        emit launchProgress(instanceId, QStringLiteral("Resolving libraries..."));
        GlobalEvents::instance().stageProgress(QStringLiteral("Resolving libraries..."));

        auto *dm = new DownloadManager(this);

        for (const auto &lib: versionInfo.libraries) {
            cacheMgr_->libraries().ensureCached(lib, *dm);
        }

        if (!versionInfo.clientJarUrl.isEmpty()) {
            auto *inst = instanceMgr_->findById(instanceId);
            QString clientJarPath = PathManager::instance().versionsDir() +
                                    QStringLiteral("/%1/%1.jar").arg(inst->mcVersion);

            if (!QFile::exists(clientJarPath)) {
                dm->enqueue(versionInfo.clientJarUrl, clientJarPath,
                            versionInfo.clientJarSha1, versionInfo.clientJarSize);
            }
        }

        if (!versionInfo.assetIndexUrl.isEmpty()) {
            QString assetIndexPath = PathManager::instance().assetsDir() +
                                     QStringLiteral("/indexes/%1.json").arg(versionInfo.assetsVersion);

            if (!QFile::exists(assetIndexPath)) {
                dm->enqueue(versionInfo.assetIndexUrl, assetIndexPath,
                            versionInfo.assetIndexSha1);
            }
        }

        if (dm->totalCount() == 0) {
            dm->deleteLater();

            QString assetIndexPath = PathManager::instance().assetsDir() +
                                     QStringLiteral("/indexes/%1.json").arg(versionInfo.assetsVersion);
            if (QFile::exists(assetIndexPath)) {
                QFile f(assetIndexPath);
                if (f.open(QIODevice::ReadOnly)) {
                    auto assets = AssetCache::parseAssetIndex(f.readAll());
                    auto *dm2 = new DownloadManager(this);
                    cacheMgr_->assets().ensureCached(assets, *dm2);

                    if (dm2->totalCount() > 0) {
                        QString msg = QStringLiteral("Downloading %1 assets...").arg(dm2->totalCount());
                        emit launchProgress(instanceId, msg);
                        GlobalEvents::instance().downloadStarted(msg);

                        connect(dm2, &DownloadManager::overallProgress, this, [instanceId](int completed, int total) {
                            int pct = total > 0 ? (completed * 100) / total : 0;
                            GlobalEvents::instance().downloadProgress(
                                pct, QStringLiteral("Assets: %1/%2").arg(completed).arg(total));
                        });

                        connect(dm2, &DownloadManager::allFinished, this,
                                [this, dm2, instanceId, versionInfo](bool success, const QStringList &errors) {
                                    dm2->deleteLater();
                                    if (!success) {
                                        X4_WARN("Launch",
                                                QStringLiteral("Some assets failed: %1").arg(errors.join(", ")));
                                    }
                                    doLaunch(instanceId, versionInfo);
                                });
                        dm2->startAll();
                        return;
                    }
                    dm2->deleteLater();
                }
            }

            doLaunch(instanceId, versionInfo);
            return;
        }

        GlobalEvents::instance().downloadStarted(QStringLiteral("Downloading %1 file(s)...").arg(dm->totalCount()));

        connect(dm, &DownloadManager::overallProgress, this,
                [this, instanceId](int completed, int total) {
                    QString msg = QStringLiteral("Downloading: %1/%2").arg(completed).arg(total);
                    emit launchProgress(instanceId, msg);
                    int pct = total > 0 ? (completed * 100) / total : 0;
                    GlobalEvents::instance().downloadProgress(pct, msg);
                });

        connect(dm, &DownloadManager::allFinished, this,
                [this, dm, instanceId, versionInfo](bool success, const QStringList &errors) {
                    dm->deleteLater();

                    if (!success && !errors.isEmpty()) {
                        X4_WARN("Launch", QStringLiteral("Download errors: %1").arg(errors.join("; ")));
                    }

                    QString assetIndexPath = PathManager::instance().assetsDir() +
                                             QStringLiteral("/indexes/%1.json").arg(versionInfo.assetsVersion);
                    if (QFile::exists(assetIndexPath)) {
                        QFile f(assetIndexPath);
                        if (f.open(QIODevice::ReadOnly)) {
                            auto assets = AssetCache::parseAssetIndex(f.readAll());
                            auto *dm2 = new DownloadManager(this);
                            cacheMgr_->assets().ensureCached(assets, *dm2);

                            if (dm2->totalCount() > 0) {
                                QString msg = QStringLiteral("Downloading %1 assets...").arg(dm2->totalCount());
                                emit launchProgress(instanceId, msg);
                                GlobalEvents::instance().downloadStarted(msg);

                                connect(dm2, &DownloadManager::overallProgress, this,
                                        [instanceId](int completed, int total) {
                                            int pct = total > 0 ? (completed * 100) / total : 0;
                                            GlobalEvents::instance().downloadProgress(
                                                pct, QStringLiteral("Assets: %1/%2").arg(completed).arg(total));
                                        });

                                connect(dm2, &DownloadManager::allFinished, this,
                                        [this, dm2, instanceId, versionInfo](bool, const QStringList &) {
                                            dm2->deleteLater();
                                            doLaunch(instanceId, versionInfo);
                                        });
                                dm2->startAll();
                                return;
                            }
                            dm2->deleteLater();
                        }
                    }

                    GlobalEvents::instance().downloadFinished(true, QString());
                    doLaunch(instanceId, versionInfo);
                });

        dm->startAll();
    }

    void LaunchEngine::doLaunch(const QString &instanceId, const VersionInfo &versionInfo) {
        auto *inst = instanceMgr_->findById(instanceId);
        if (!inst) {
            emit launchFailed(instanceId, QStringLiteral("Instance disappeared during launch"));
            return;
        }

        const auto *manifestEntry = manifest_->find(inst->mcVersion);
        if (!manifestEntry) {
            emit launchFailed(
                instanceId,
                QStringLiteral("GAME_LAUNCH_ERROR: Minecraft version %1 not found in manifest.").arg(inst->mcVersion));
            return;
        }
        // legacy version types (old_alpha/old_beta) are handled as normal releases

        auto config = instanceMgr_->loadConfig(instanceId);

        emit launchProgress(instanceId, QStringLiteral("Finding Java..."));
        GlobalEvents::instance().stageProgress(QStringLiteral("Finding Java..."));

        JavaVersion java;
        const QString configJavaPath = config.effectiveJavaPath().trimmed();
        if (!configJavaPath.isEmpty()) {
            auto candidate = JavaDetector::probeJavaBinary(configJavaPath);
            if (!candidate.isValid()) {
                emit launchFailed(
                    instanceId,
                    QStringLiteral("GAME_LAUNCH_ERROR: Configured Java path is invalid: %1").arg(configJavaPath));
                return;
            }
            const int required = JavaManager::requiredJavaMajor(inst->mcVersion);
            if (candidate.major < required) {
                emit launchFailed(
                    instanceId, QStringLiteral(
                        "GAME_LAUNCH_ERROR: Configured Java (%1) is too old for MC %2 (needs Java %3+).")
                    .arg(candidate.versionString(), inst->mcVersion).arg(required));
                return;
            }
            java = candidate;
        } else {
            java = javaMgr_->findJavaForMC(inst->mcVersion);
            if (java.isValid()) {
                X4_INFO("Launch", QStringLiteral("Auto-detected Java %1 for MC %2 at: %3")
                        .arg(java.major).arg(inst->mcVersion, java.path));
            }
        }

        if (!java.isValid()) {
            // Try fallback: probe `java` on PATH as last attempt
            QString pathJava = QStandardPaths::findExecutable(QStringLiteral("java"));
            if (!pathJava.isEmpty()) {
                auto candidate = JavaDetector::probeJavaBinary(pathJava);
                if (candidate.isValid() && candidate.major >= JavaManager::requiredJavaMajor(inst->mcVersion)) {
                    java = candidate;
                    X4_INFO("Launch", QStringLiteral("Using fallback java on PATH: %1").arg(pathJava));
                }
            }
        }

        if (!java.isValid()) {
            emit launchFailed(instanceId,
                              QStringLiteral(
                                  "GAME_LAUNCH_ERROR: No suitable Java found for Minecraft %1 (needs Java %2+)")
                              .arg(inst->mcVersion).arg(JavaManager::requiredJavaMajor(inst->mcVersion)));
            return;
        }

        emit launchProgress(instanceId, QStringLiteral("Preparing launch..."));
        GlobalEvents::instance().stageProgress(QStringLiteral("Preparing launch..."));
        // Prepare a safe gameDir; for some legacy loaders (Forge/NeoForge) paths
        // containing characters like '+' or '!' are rejected. Try to create a
        // safe symlink and use it as the working directory instead of failing.
        QString safeGameDir;
        QString origGameDir = instanceMgr_->minecraftDir(instanceId);
        if (inst->loader == ModLoaderType::Forge || inst->loader == ModLoaderType::NeoForge) {
            if (origGameDir.contains(QStringLiteral("+")) || origGameDir.contains(QStringLiteral("!"))) {
                QString tmpBase = PathManager::instance().dataDir() + QStringLiteral("/tmp_launch");
                QDir().mkpath(tmpBase);
                QString linkPath = tmpBase + QStringLiteral("/instance_") + instanceId;
                QFile::remove(linkPath);
                bool created = false;
                if (QFile::link(origGameDir, linkPath)) {
                    created = true;
                    X4_INFO("Launch",
                            QStringLiteral("QFile::link created symlink %1 -> %2").arg(linkPath, origGameDir));
                } else {
#ifdef Q_OS_WIN
                    // On Windows, use directory junction (no admin required, unlike symlinks)
                    int rc = QProcess::execute(QStringLiteral("cmd"),
                        {QStringLiteral("/c"), QStringLiteral("mklink"),
                         QStringLiteral("/J"), QDir::toNativeSeparators(linkPath),
                         QDir::toNativeSeparators(origGameDir)});
                    X4_INFO("Launch", QStringLiteral("mklink /J rc=%1 %2 -> %3").arg(rc).arg(linkPath, origGameDir));
#else
                    int rc = QProcess::execute(QStringLiteral("ln"), {QStringLiteral("-s"), origGameDir, linkPath});
                    X4_INFO("Launch", QStringLiteral("ln -s rc=%1 args=%2").arg(rc).arg(origGameDir + " " + linkPath));
#endif
                    if (rc == 0) created = true;
                }
                if (created) {
                    safeGameDir = linkPath;
                    X4_INFO("Launch",
                            QStringLiteral("Created safe symlink for instance %1 -> %2").arg(linkPath, origGameDir));
                } else {
                    X4_WARN("Launch", QStringLiteral(
                                "Forge path contains invalid characters and safe symlink creation failed: %1").arg(
                                origGameDir));
                    emit launchFailed(instanceId,
                                      QStringLiteral(
                                          "Forge cannot launch from a folder containing characters like '+' or '!'.\n\n"
                                          "Your instance path is: %1").arg(origGameDir));
                    return;
                }
            }
        }

        QStringList classpath;
        for (const auto &lib: versionInfo.libraries) {
            if (lib.isNative) continue;
            QString path = cacheMgr_->libraries().getPath(lib);
            if (!path.isEmpty()) {
                classpath << path;
            } else {
                X4_WARN("Launch", QStringLiteral("Warning: Missing required library %1").arg(lib.name));
            }
        }

        QString clientJar = PathManager::instance().versionsDir() +
                            QStringLiteral("/%1/%1.jar").arg(inst->mcVersion);
        if (QFile::exists(clientJar) && QFile(clientJar).size() > 0) {
            classpath << clientJar;
        } else {
            emit launchFailed(instanceId, QStringLiteral("Client JAR is missing or empty. Download failed."));
            return;
        }


        QString nativesDir = PathManager::instance().nativesDir() +
                             QStringLiteral("/") + instanceId;
        QDir().mkpath(nativesDir);
        QString nativeErr;
        if (!extractNatives(nativesDir, versionInfo.libraries, &nativeErr)) {
            QDir(nativesDir).removeRecursively();
            emit launchFailed(instanceId, QStringLiteral("GAME_LAUNCH_ERROR: %1").arg(nativeErr));
            return;
        }

#ifdef Q_OS_LINUX
        if (QStandardPaths::findExecutable(QStringLiteral("xrandr")).isEmpty()) {
            const QString nativesBase = PathManager::instance().nativesDir();
            if (!isSafeChildPath(nativesBase, nativesDir)) {
                X4_WARN("Launch",
                        QStringLiteral("Refusing to create dummy xrandr: unsafe nativesDir %1").arg(nativesDir));
            } else {
                QString fakeXrandr = nativesDir + QStringLiteral("/xrandr");
                QFile fx(fakeXrandr);
                if (!fx.exists() && fx.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    fx.write("#!/bin/sh\n");
                    fx.write("echo 'Screen 0: minimum 320 x 200, current 1920 x 1080, maximum 16384 x 16384'\n");
                    fx.write("echo 'default connected primary 1920x1080+0+0 0mm x 0mm'\n");
                    fx.write("echo '   1920x1080     60.00* '\n");
                    fx.close();
                    fx.setPermissions(
                        fx.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeUser | QFileDevice::ExeGroup |
                        QFileDevice::ExeOther);
                    X4_INFO("Launch", QStringLiteral("Created dummy xrandr script to fix Wayland crash"));
                }
            }
        }
#endif

        // Use origGameDir or safeGameDir (set earlier if created)
        QString gameDir = !safeGameDir.isEmpty() ? safeGameDir : origGameDir;
        QString assetsDir = cacheMgr_->assets().assetsRootDir();

        auto launchArgs = ArgumentResolver::resolve(
            versionInfo, *inst, config, java.path,
            classpath, nativesDir, gameDir, assetsDir);

        QStringList fullArgs;
        fullArgs << launchArgs.jvmArgs;
        fullArgs << launchArgs.mainClass;
        fullArgs << launchArgs.gameArgs;

        const QString preflightError = validateLaunchInputs(
            versionInfo, java, inst->mcVersion, gameDir, assetsDir, nativesDir, classpath,
            launchArgs.jvmArgs, launchArgs.gameArgs);
        if (!preflightError.isEmpty()) {
            QDir(nativesDir).removeRecursively();
            emit launchFailed(instanceId, preflightError);
            return;
        }

        X4_INFO("Launch", QStringLiteral("Launching with Java: %1").arg(java.path));
        X4_INFO("Launch", QStringLiteral("Main class: %1").arg(launchArgs.mainClass));
        X4_INFO("Launch", QStringLiteral("Classpath entries: %1").arg(classpath.size()));
        X4_DEBUG("Launch", QStringLiteral("Args: %1").arg(fullArgs.join(' ')));

        auto *proc = new QProcess(nullptr);
        proc->setWorkingDirectory(gameDir);
        proc->setProcessChannelMode(QProcess::MergedChannels);

        auto env = QProcessEnvironment::systemEnvironment();
#ifdef Q_OS_LINUX
        QString display = env.value(QStringLiteral("DISPLAY")).trimmed();

        const bool wayland = isWaylandSession(env);
        if (wayland) {
            X11Binding binding = resolveUsableX11Binding(env, display);
            if (binding.display.isEmpty()) {
                const bool allowEmbeddedXwayland =
                        env.value(QStringLiteral("X4_ALLOW_EMBEDDED_XWAYLAND")) == QStringLiteral("1");
                if (allowEmbeddedXwayland) {
                    QString startedDisplay;
                    QString startReason;
                    X4_WARN("Launch", QStringLiteral(
                                "No usable compositor XWayland found. Trying embedded Xwayland (debug mode)..."));
                    if (tryStartXWaylandServer(env, &startedDisplay, &startReason)) {
                        env.insert(QStringLiteral("DISPLAY"), startedDisplay);
                        binding = resolveUsableX11Binding(env, startedDisplay);
                        if (binding.display.isEmpty()) {
                            binding.display = startedDisplay;
                            binding.xauthority.clear();
                        }
                        X4_INFO("Launch", QStringLiteral("Started embedded Xwayland on %1").arg(startedDisplay));
                    } else {
                        QDir(nativesDir).removeRecursively();
                        instanceMgr_->setInstanceState(instanceId, InstanceState::Error);
                        const QString msg = QStringLiteral(
                                    "GAME_LAUNCH_ERROR[1001]: Wayland detected but no usable XWayland DISPLAY was found (%1)")
                                .arg(startReason);
                        X4_ERROR("Launch", msg);
                        emit launchFailed(instanceId, msg);
                        return;
                    }
                } else {
                    QDir(nativesDir).removeRecursively();
                    instanceMgr_->setInstanceState(instanceId, InstanceState::Error);
                    const QString msg = QStringLiteral(
                        "GAME_LAUNCH_ERROR[1001]: Wayland detected but compositor XWayland DISPLAY is unavailable. "
                        "Enable XWayland in compositor or set X4_ALLOW_EMBEDDED_XWAYLAND=1 for fallback.");
                    X4_ERROR("Launch", msg);
                    emit launchFailed(instanceId, msg);
                    return;
                }
            }

            env.insert(QStringLiteral("DISPLAY"), binding.display);
            if (!binding.xauthority.isEmpty()) {
                env.insert(QStringLiteral("XAUTHORITY"), binding.xauthority);
            } else {
                env.remove(QStringLiteral("XAUTHORITY"));
            }
            env.insert(QStringLiteral("_JAVA_AWT_WM_NONREPARENTING"), QStringLiteral("1"));
            env.remove(QStringLiteral("WAYLAND_DISPLAY"));
            env.remove(QStringLiteral("WAYLAND_SOCKET"));
            env.insert(QStringLiteral("XDG_SESSION_TYPE"), QStringLiteral("x11"));
            env.insert(QStringLiteral("GDK_BACKEND"), QStringLiteral("x11"));
            env.insert(QStringLiteral("SDL_VIDEODRIVER"), QStringLiteral("x11"));
            env.insert(QStringLiteral("CLUTTER_BACKEND"), QStringLiteral("x11"));
            X4_INFO("Launch",
                    QStringLiteral("Wayland detected -> forcing XWayland on DISPLAY %1").arg(binding.display));
        } else {
            const QString explicitXauth = env.value(QStringLiteral("XAUTHORITY")).trimmed();
            if (display.isEmpty() || !canConnectToDisplay(env, display, explicitXauth)) {
                QDir(nativesDir).removeRecursively();
                instanceMgr_->setInstanceState(instanceId, InstanceState::Error);
                const QString msg = QStringLiteral("GAME_LAUNCH_ERROR: No usable X11 DISPLAY available.");
                X4_ERROR("Launch", msg);
                emit launchFailed(instanceId, msg);
                return;
            }
        }

        env.insert(QStringLiteral("QT_X11_NO_MITSHM"), QStringLiteral("1"));
        env.insert(QStringLiteral("PATH"), nativesDir + QStringLiteral(":") + env.value(QStringLiteral("PATH")));
#endif
        proc->setProcessEnvironment(env);

#ifdef Q_OS_WIN
        proc->setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args) {
            args->flags |= CREATE_NO_WINDOW;
        });
#else
        proc->setChildProcessModifier([]() {
            ::setsid();
        });
#endif

        connect(proc, &QProcess::readyReadStandardOutput, this,
                [this, proc, instanceId]() {
                    while (proc->canReadLine()) {
                        QString line = QString::fromUtf8(proc->readLine()).trimmed();
                        if (!line.isEmpty()) {
                            X4_INFO("Game", line);
                            auto &lines = recentOutputLines_[instanceId];
                            lines.append(line);
                            if (lines.size() > 200) {
                                lines.remove(0, lines.size() - 200);
                            }
                            emit gameOutput(instanceId, line);
                        }
                    }
                });

        connect(proc, &QProcess::finished, this,
                [this, proc, instanceId, nativesDir, safeGameDir](int exitCode, QProcess::ExitStatus) {
                    runningProcesses_.remove(instanceId);
                    proc->deleteLater();
                    QDir(nativesDir).removeRecursively();
                    if (!safeGameDir.isEmpty()) {
                        // remove symlink created for safe launch
                        QFile::remove(safeGameDir);
                        X4_INFO("Launch", QStringLiteral("Removed safe symlink: %1").arg(safeGameDir));
                    }
                    const QStringList lines = recentOutputLines_.take(instanceId);
                    const QDateTime startedAt = processStartTimes_.take(instanceId);
                    auto *inst = instanceMgr_->findById(instanceId);
                    if (inst && startedAt.isValid()) {
                        inst->playTime += startedAt.secsTo(QDateTime::currentDateTime());
                        instanceMgr_->saveInstance(*inst);
                    }

                    if (exitCode == 0) {
                        instanceMgr_->setInstanceState(instanceId, InstanceState::Ready);
                    } else {
                        instanceMgr_->setInstanceState(instanceId, InstanceState::Error);
                        const QString knownReason = detectKnownCrashReason(lines);
                        const QString gameDir = inst ? instanceMgr_->minecraftDir(instanceId) : QString();
                        QString crashReportPath = extractCrashReportFromOutput(lines);
                        if (crashReportPath.isEmpty() && !gameDir.isEmpty()) {
                            crashReportPath = findRecentCrashReport(gameDir, startedAt);
                        }
                        const QString summary = buildCrashSummaryMessage(exitCode, knownReason, crashReportPath);
                        X4_ERROR("Launch", summary);
                        emit launchFailed(instanceId, summary);
                    }
                    X4_INFO("Launch", QStringLiteral("Game exited with code %1").arg(exitCode));
                    emit gameFinished(instanceId, exitCode);
                });

        connect(proc, &QProcess::errorOccurred, this,
                [this, proc, instanceId](QProcess::ProcessError error) {
                    if (error == QProcess::FailedToStart) {
                        runningProcesses_.remove(instanceId);
                        recentOutputLines_.remove(instanceId);
                        proc->deleteLater();
                        instanceMgr_->setInstanceState(instanceId, InstanceState::Error);
                        emit launchFailed(instanceId, QStringLiteral("Failed to start Java process"));
                    }
                });
        connect(proc, &QProcess::started, this, [this, instanceId]() {
            auto *runningInst = instanceMgr_->findById(instanceId);
            if (runningInst) {
                runningInst->lastPlayed = QDateTime::currentDateTime();
                instanceMgr_->setInstanceState(instanceId, InstanceState::Running);
            }
            processStartTimes_[instanceId] = QDateTime::currentDateTime();
            emit launchStarted(instanceId);
        });
        proc->start(java.path, fullArgs);
        emit launchProgress(instanceId, QStringLiteral("Game running..."));
        GlobalEvents::instance().downloadFinished(true, QString());
        runningProcesses_[instanceId] = proc;
    }

    bool LaunchEngine::extractNatives(const QString &nativesDir, const QVector<LibraryInfo> &libs, QString *error) {
        for (const auto &lib: libs) {
            if (!lib.isNative) continue;

            QString jarPath = cacheMgr_->libraries().getPath(lib);
            if (jarPath.isEmpty() || !QFileInfo::exists(jarPath)) {
                if (error) {
                    *error = QStringLiteral("Missing native library: %1").arg(lib.name);
                }
                return false;
            }
            // Extract into a temporary directory first to prevent zip-slip into arbitrary paths.
            QTemporaryDir tmpDir;
            if (!tmpDir.isValid()) {
                if (error) *error = QStringLiteral("Failed to create temporary directory for native extraction");
                return false;
            }

            QProcess proc;
            proc.setWorkingDirectory(tmpDir.path());

#ifdef Q_OS_WIN
            proc.start(QStringLiteral("tar"), {QStringLiteral("-xf"), jarPath});
#else
            proc.start(QStringLiteral("unzip"), {
                           QStringLiteral("-o"), QStringLiteral("-q"), jarPath, QStringLiteral("-d"), tmpDir.path()
                       });
#endif
            if (!proc.waitForFinished(15000) || proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
                if (error) {
                    *error = QStringLiteral("Failed to extract natives from %1").arg(jarPath);
                }
                return false;
            }

            // Move extracted content into nativesDir while validating paths.
            QDirIterator it(tmpDir.path(), QDir::NoDotAndDotDot | QDir::AllEntries, QDirIterator::Subdirectories);
            const QString nativesBase = PathManager::instance().nativesDir();
            if (!isSafeChildPath(nativesBase, nativesDir)) {
                if (error) *error = QStringLiteral("Unsafe nativesDir location: %1").arg(nativesDir);
                return false;
            }

            while (it.hasNext()) {
                const QString src = it.next();
                const QString rel = QDir(tmpDir.path()).relativeFilePath(src);
                // Skip META-INF and other metadata entries; they are not needed in natives dir.
                if (rel.startsWith(QStringLiteral("META-INF"), Qt::CaseSensitive) || rel.startsWith(
                        QStringLiteral("__MACOSX"))) {
                    continue;
                }
                const QString dest = QDir(nativesDir).filePath(rel);

                // Ensure destination will be inside nativesBase
                if (!isSafeChildPath(nativesBase, dest)) {
                    if (error) *error = QStringLiteral("Extracted entry would escape natives directory: %1").arg(rel);
                    return false;
                }

                QFileInfo sfi(src);
                if (sfi.isDir()) {
                    QDir().mkpath(dest);
                } else {
                    QDir().mkpath(QFileInfo(dest).absolutePath());
                    // Try rename first (fast), fall back to copy if across filesystems
                    if (!QFile::rename(src, dest)) {
                        if (!QFile::copy(src, dest)) {
                            if (error) *error = QStringLiteral("Failed to move extracted native file: %1").arg(rel);
                            return false;
                        }
                        QFile::remove(src);
                    }
                }
            }
        }

        QDir metaInf(nativesDir + QStringLiteral("/META-INF"));
        if (metaInf.exists()) {
            // safe removal check
            const QString nativesBase = PathManager::instance().nativesDir();
            if (!safeRemoveRecursively(nativesBase, metaInf.absolutePath())) {
                if (error) *error = QStringLiteral("Refused to remove META-INF: unsafe path");
                return false;
            }
        }
        return true;
    }
}