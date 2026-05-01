#include "JavaDetector.h"
#include "../core/PathManager.h"
#include "../core/Logger.h"
#include <QProcess>
#include <QDir>
#include <QDirIterator>
#include <QRegularExpression>
#include <QtConcurrent>

namespace x4 {

JavaDetector::JavaDetector(QObject* parent) : QObject(parent) {}

void JavaDetector::detectAll() {
    
    auto future = QtConcurrent::run([this]() -> QVector<JavaVersion> {
        QVector<JavaVersion> found;
        QSet<QString> seen; 

        auto paths = searchPaths();
        for (const auto& searchDir : paths) {
            QDir dir(searchDir);
            if (!dir.exists()) continue;

            
            QDirIterator it(searchDir, QDir::Dirs | QDir::NoDotAndDotDot);
            while (it.hasNext()) {
                it.next();
                QString binDir = it.filePath() + QStringLiteral("/bin");
#ifdef Q_OS_WIN
                QString javaExe = binDir + QStringLiteral("/java.exe");
#else
                QString javaExe = binDir + QStringLiteral("/java");
#endif
                if (QFileInfo::exists(javaExe)) {
                    QString canonical = QFileInfo(javaExe).canonicalFilePath();
                    if (seen.contains(canonical)) continue;
                    seen.insert(canonical);

                    auto ver = probeJavaBinary(javaExe);
                    if (ver.isValid()) {
                        found.append(ver);
                    }
                }
            }
        }

        
        QString pathJava = QStandardPaths::findExecutable(QStringLiteral("java"));
        if (!pathJava.isEmpty()) {
            QString canonical = QFileInfo(pathJava).canonicalFilePath();
            if (!seen.contains(canonical)) {
                auto ver = probeJavaBinary(pathJava);
                if (ver.isValid()) found.append(ver);
            }
        }

        
        std::sort(found.begin(), found.end(), [](const JavaVersion& a, const JavaVersion& b) {
            return a.major > b.major;
        });

        return found;
    });

    
    auto* watcher = new QFutureWatcher<QVector<JavaVersion>>(this);
    connect(watcher, &QFutureWatcher<QVector<JavaVersion>>::finished, this, [this, watcher]() {
        results_ = watcher->result();
        X4_INFO("Java", QStringLiteral("Detected %1 Java installation(s)").arg(results_.size()));
        emit detected(results_);
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}

JavaVersion JavaDetector::probeJavaBinary(const QString& javaPath) {
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(javaPath, {QStringLiteral("-version")});

    if (!proc.waitForFinished(5000)) {
        return {};
    }

    QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());
    if (output.isEmpty())
        output = QString::fromLocal8Bit(proc.readAllStandardError());

    
    static QRegularExpression re(QStringLiteral(R"(version\s+"(\d+)(?:\.(\d+))?(?:\.(\d+))?[^"]*")"));
    auto match = re.match(output);

    if (!match.hasMatch()) return {};

    JavaVersion ver;
    ver.major = match.captured(1).toInt();
    ver.minor = match.captured(2).toInt();
    ver.patch = match.captured(3).toInt();
    ver.path = QFileInfo(javaPath).canonicalFilePath();

    
    if (ver.major == 1 && ver.minor <= 8) {
        ver.major = ver.minor;
        ver.minor = ver.patch;
        ver.patch = 0;
    }

    
    if (output.contains(QStringLiteral("64-Bit")) || output.contains(QStringLiteral("amd64")) ||
        output.contains(QStringLiteral("x86_64"))) {
        ver.arch = QStringLiteral("x64");
    } else if (output.contains(QStringLiteral("aarch64"))) {
        ver.arch = QStringLiteral("aarch64");
    } else {
        ver.arch = QStringLiteral("x86");
    }

    
    ver.isManaged = ver.path.startsWith(PathManager::instance().javaDir());

    return ver;
}

QStringList JavaDetector::searchPaths() const {
    QStringList paths;

    
    paths << PathManager::instance().javaDir();

#ifdef Q_OS_LINUX
    paths << QStringLiteral("/usr/lib/jvm");
    paths << QStringLiteral("/usr/java");
    paths << QStringLiteral("/usr/local/java");
    QString home = QDir::homePath();
    paths << home + QStringLiteral("/.sdkman/candidates/java");
    paths << home + QStringLiteral("/.jdks");
#elif defined(Q_OS_WIN)
    paths << QStringLiteral("C:/Program Files/Java");
    paths << QStringLiteral("C:/Program Files/Eclipse Adoptium");
    paths << QStringLiteral("C:/Program Files/Microsoft");
    paths << QStringLiteral("C:/Program Files/Zulu");
    paths << QStringLiteral("C:/Program Files/BellSoft");
    paths << QStringLiteral("C:/Program Files/Amazon Corretto");
    paths << QStringLiteral("C:/Program Files (x86)/Java");
    // User-local install locations
    QString localAppData = qEnvironmentVariable("LOCALAPPDATA");
    if (!localAppData.isEmpty()) {
        paths << localAppData + QStringLiteral("/Programs/Eclipse Adoptium");
    }
    
    QString javaHome = qEnvironmentVariable("JAVA_HOME");
    if (!javaHome.isEmpty()) {
        paths << javaHome;
        paths << QFileInfo(javaHome).absolutePath();
    }
#endif

    return paths;
}

} 
